# Табличный процессор

## Диаграмма классов

Я разбил диаграмму на несколько частей, потому что одна общая получилась слишком большой.
### Адреса и значения

`CellAddress` это просто номер строки и столбца. `CellRef` это ссылка из формулы, в ней ещё есть флаги для `$`. На первом этапе они не нужны, но пригодятся при копировании.

Значения сделаны через наследование от `Value`. Так проще добавить новый тип, например дату. Ошибки вроде `#DIV/0!` тоже считаются значением, чтобы ошибка в одной ячейке не ломала остальные.

```mermaid
classDiagram
    class CellAddress {
        +int row
        +int col
        +parse(string_view) CellAddress$
        +toString() string
    }
    class CellRef {
        +CellAddress addr
        +bool absRow
        +bool absCol
        +shifted(int dr, int dc) CellRef
    }
    class Range {
        +CellAddress from
        +CellAddress to
        +contains(CellAddress) bool
        +forEach(fn) void
    }
    CellRef --> CellAddress
    Range --> CellAddress

    class Value {
        <<abstract>>
        +asNumber() optional~double~
        +toText() string
        +isError() bool
        +display(NumberFormat) string
    }
    class EmptyValue
    class NumberValue {
        -double v
    }
    class StringValue {
        -string s
    }
    class ErrorValue {
        -ErrorCode code
    }
    class DateValue {
        -int days
    }
    class ErrorCode {
        <<enumeration>>
        DIV0
        REF
        NAME
        CYCLE
        VALUE
    }
    Value <|-- EmptyValue
    Value <|-- NumberValue
    Value <|-- StringValue
    Value <|-- ErrorValue
    Value <|-- DateValue : этап 2
    ErrorValue --> ErrorCode
```

Дату буду хранить как количество дней, тогда разность дат считается обычным вычитанием.

### Формулы

Формула после разбора хранится как дерево. Например, `=A1*2+SUM(B1:B3)`: сверху плюс, слева от него умножение, справа функция.

С этим деревом надо делать несколько разных вещей: считать значение, печатать формулу обратно в текст, искать ссылки на другие ячейки. Каждое такое действие я вынес в отдельный класс-посетитель.

```mermaid
classDiagram
    class Expr {
        <<abstract>>
        +accept(ExprVisitor) void
        +clone() unique_ptr~Expr~
    }
    class NumberLit
    class StringLit
    class RefExpr {
        +CellRef ref
    }
    class RangeExpr {
        +CellRef from
        +CellRef to
    }
    class UnaryExpr {
        +UnaryOp op
        +unique_ptr~Expr~ operand
    }
    class BinaryExpr {
        +BinaryOp op
        +unique_ptr~Expr~ left
        +unique_ptr~Expr~ right
    }
    class FunctionCallExpr {
        +string name
        +vector~unique_ptr~Expr~~ args
        +unique_ptr~Function~ fn
    }
    Expr <|-- NumberLit
    Expr <|-- StringLit
    Expr <|-- RefExpr
    Expr <|-- RangeExpr
    Expr <|-- UnaryExpr
    Expr <|-- BinaryExpr
    Expr <|-- FunctionCallExpr
    UnaryExpr *-- Expr
    BinaryExpr *-- Expr
    FunctionCallExpr *-- Expr
    FunctionCallExpr *-- Function

    class ExprVisitor {
        <<interface>>
        +visit(NumberLit)
        +visit(StringLit)
        +visit(RefExpr)
        +visit(RangeExpr)
        +visit(UnaryExpr)
        +visit(BinaryExpr)
        +visit(FunctionCallExpr)
    }
    class Evaluator {
        -EvalContext ctx
        +result() shared_ptr~const Value~
    }
    class FormulaPrinter {
        +result() string
    }
    class RefCollector {
        +result() set~CellAddress~
    }
    class RefShifter {
        -ShiftRule rule
    }
    ExprVisitor <|.. Evaluator
    ExprVisitor <|.. FormulaPrinter
    ExprVisitor <|.. RefCollector
    ExprVisitor <|.. RefShifter : этап 2
    Expr ..> ExprVisitor
```

### Парсер и функции

Парсер пишу сам. `Lexer` делит строку на токены (числа, ссылки, знаки), а `Parser` строит из них дерево методом рекурсивного спуска. Каждый метод отвечает за свой приоритет операций: сравнения, потом `+ -`, потом `* /`, потом унарный минус.

Парсер не знает, какие функции существуют. Когда он видит имя, например `SUM`, он спрашивает у `FunctionRegistry`. Если функции с таким именем нет, получится `#NAME?`.

Аргументы функция получает ещё не посчитанными. Это нужно для `IF`: в `=IF(A1=0, 0, 1/A1)` деление не должно считаться, если A1 равно 0.

```mermaid
classDiagram
    class Token {
        +TokenType type
        +string text
        +size_t pos
    }
    class Lexer {
        +Lexer(string_view)
        +next() Token
    }
    class Parser {
        -Lexer lexer
        -const FunctionRegistry& functions
        +parse(string_view) unique_ptr~Expr~
        -comparison() unique_ptr~Expr~
        -additive() unique_ptr~Expr~
        -multiplicative() unique_ptr~Expr~
        -unary() unique_ptr~Expr~
        -primary() unique_ptr~Expr~
    }
    Parser *-- Lexer
    Lexer ..> Token
    Parser --> FunctionRegistry

    class Function {
        <<abstract>>
        +name() string_view
        +call(span~const Expr*~ args, EvalContext ctx) shared_ptr~const Value~
    }
    class SumFn
    class MinFn
    class MaxFn
    class AverageFn
    class IfFn
    Function <|-- SumFn
    Function <|-- MinFn
    Function <|-- MaxFn
    Function <|-- AverageFn
    Function <|-- IfFn

    class FunctionRegistry {
        -map~string, Factory~ factories
        +add(string name, Factory f) void
        +create(string_view name) unique_ptr~Function~
        +withBuiltins() FunctionRegistry$
    }
    FunctionRegistry ..> Function : создаёт

    class EvalContext {
        <<interface>>
        +valueAt(CellAddress) shared_ptr~const Value~
        +evaluate(const Expr&) shared_ptr~const Value~
    }
    Function ..> EvalContext
```

### Лист, граф зависимостей и файлы

`Sheet` хранит только заполненные ячейки, пустые не хранятся.

`DependencyGraph` помнит, какая ячейка от какой зависит. По нему видно, что надо пересчитать после изменения ячейки.

`Spreadsheet` это главный класс ядра. Всё остальное (лист, граф, парсер) используется через него.

```mermaid
classDiagram
    class Cell {
        +string raw
        +unique_ptr~Expr~ formula
        +shared_ptr~const Value~ value
        +CellStyle style
    }
    class Sheet {
        -unordered_map~CellAddress, Cell~ cells
        +get(CellAddress) const Cell*
        +set(CellAddress, Cell) void
        +remove(CellAddress) void
        +usedRange() Range
    }
    Sheet *-- Cell

    class DependencyGraph {
        -map~CellAddress, set~CellAddress~~ deps
        -map~CellAddress, set~CellAddress~~ dependents
        +setDependencies(CellAddress, set~CellAddress~) void
        +affectedInOrder(CellAddress) vector~CellAddress~
        +cycleThrough(CellAddress) set~CellAddress~
    }

    class Spreadsheet {
        -Sheet sheet
        -DependencyGraph graph
        -const FunctionRegistry& functions
        -vector~CellObserver*~ observers
        +setInput(CellAddress, string_view) void
        +clear(CellAddress) void
        +value(CellAddress) shared_ptr~const Value~
        +raw(CellAddress) string
        +subscribe(CellObserver*) void
        +unsubscribe(CellObserver*) void
    }
    Spreadsheet *-- Sheet
    Spreadsheet *-- DependencyGraph
    Spreadsheet --> FunctionRegistry
    Spreadsheet ..|> EvalContext
    Spreadsheet o-- CellObserver

    class CellObserver {
        <<interface>>
        +onCellsChanged(span~const CellAddress~) void
    }

    class FileFormat {
        <<interface>>
        +name() string_view
        +extensions() span~const string~
        +load(istream, Spreadsheet) void
        +save(ostream, const Spreadsheet) void
    }
    class NativeFormat
    class CsvFormat
    FileFormat <|.. NativeFormat
    FileFormat <|.. CsvFormat
    class FormatRegistry {
        +add(unique_ptr~FileFormat~) void
        +byExtension(string_view) const FileFormat*
    }
    FormatRegistry *-- FileFormat

    class NumberFormat {
        <<interface>>
        +format(double) string
    }
    class GeneralFormat
    class FixedFormat
    class PercentFormat
    NumberFormat <|.. GeneralFormat
    NumberFormat <|.. FixedFormat : этап 2
    NumberFormat <|.. PercentFormat : этап 2
```

### Приложение и интерфейс

`Session` хранит всё, что относится к текущей работе: документ, курсор, режим, сообщение в строке состояния. Нажатую клавишу она передаёт текущему режиму, потому что в разных режимах одна клавиша делает разное.

Какая клавиша что делает, написано в `Keymap`, он читается из конфига. Там клавише соответствует имя действия, а `ActionRegistry` создаёт по этому имени объект `Action`.

```mermaid
classDiagram
    class Session {
        -Spreadsheet doc
        -CellAddress cursor
        -Viewport viewport
        -string editBuffer
        -string status
        -unique_ptr~Mode~ mode
        -Keymap keymap
        -UndoStack undo
        -TaskRunner tasks
        +handleKey(string_view key) void
        +setMode(unique_ptr~Mode~) void
        +subscribe(SessionObserver*) void
    }
    Session *-- Spreadsheet
    Session *-- Mode
    Session *-- Keymap
    Session *-- UndoStack
    Session *-- TaskRunner
    Session ..|> CellObserver

    class Mode {
        <<abstract>>
        +name() string_view
        +handleKey(Session, string_view key) void
    }
    class NormalMode
    class EditMode
    class CommandMode
    class HelpMode
    Mode <|-- NormalMode
    Mode <|-- EditMode
    Mode <|-- CommandMode
    Mode <|-- HelpMode

    class Action {
        <<abstract>>
        +name() string_view
        +execute(Session) void
    }
    class MoveCursorAction
    class StartEditAction
    class SaveAction
    class OpenAction
    class ImportCsvAction
    class ShowHelpAction
    class QuitAction
    Action <|-- MoveCursorAction
    Action <|-- StartEditAction
    Action <|-- SaveAction
    Action <|-- OpenAction
    Action <|-- ImportCsvAction
    Action <|-- ShowHelpAction
    Action <|-- QuitAction

    class ActionRegistry {
        +add(string name, Factory f) void
        +create(string_view name) unique_ptr~Action~
    }
    class Keymap {
        -map~string, string~ keyToAction
        +actionFor(string_view key) optional~string~
        +load(path) Keymap$
        +defaults() Keymap$
    }
    NormalMode --> Keymap
    NormalMode --> ActionRegistry

    class EditCommand {
        <<abstract>>
        +apply(Spreadsheet) void
        +revert(Spreadsheet) void
    }
    class SetCellCommand
    EditCommand <|-- SetCellCommand
    class UndoStack {
        -vector~unique_ptr~EditCommand~~ done
        -vector~unique_ptr~EditCommand~~ undone
        +push(unique_ptr~EditCommand~) void
        +undo(Spreadsheet) void
        +redo(Spreadsheet) void
    }
    UndoStack *-- EditCommand

    class SessionObserver {
        <<interface>>
        +onSessionChanged() void
    }
    class TuiApp {
        -ScreenInteractive screen
        -Session& session
        +run() int
    }
    TuiApp ..|> SessionObserver
    TuiApp --> Session
    TuiApp *-- GridView
    TuiApp *-- FormulaBarView
    TuiApp *-- StatusBarView
    TuiApp *-- HelpView
```

`UndoStack` и `EditCommand` понадобятся только на втором этапе.

## Слои и зависимости между ними

Программа состоит из трёх частей, в CMake это три цели:

- `core` — ядро: ячейки, формулы, пересчёт, сохранение и загрузка. Про терминал ничего не знает. Тесты подключаются только к нему.
- `app` — логика работы с программой: курсор, режимы, клавиши, отмена.
- `spreadsheet` — сама программа с интерфейсом на FTXUI. Рисует таблицу и передаёт нажатия клавиш в `app`.

Зависимости идут только вниз: интерфейс использует `app`, `app` использует `core`. FTXUI подключен только к `spreadsheet`, поэтому `core` с ним не связан.

Клавиши передаются в `app` просто строками, например `"enter"` или `"ctrl+s"`. Тогда `app` тоже не зависит от FTXUI.

## Точки расширения

- **Новая функция в формулах.** Создать класс-наследник `Function` (например, `CountFn`) и зарегистрировать его в `FunctionRegistry` под именем `"COUNT"`. Парсер менять не надо.
- **Новый тип значения.** Создать наследника `Value` и реализовать у него `asNumber()`, `toText()` и `display()`.
- **Новый формат файла.** Создать наследника `FileFormat` с методами `load` и `save` и добавить его в `FormatRegistry`. Формат выбирается по расширению файла.
- **Новый формат чисел** (этап 2). Создать наследника `NumberFormat`.
- **Новая операция с отменой** (этап 2). Создать наследника `EditCommand` с методами `apply` и `revert`, а также `Action`, который его запускает. Потом назначить ему клавишу в конфиге.
- **Новое действие по клавише.** Создать наследника `Action`, зарегистрировать его в `ActionRegistry` и назначить клавишу в конфиге.

## Паттерны

### Composite — дерево формулы

Формула может быть вложенной, например `=IF(A1>0, SUM(B1:B3)*2, -C1)`. Поэтому я храню её деревом: у каждого узла есть свои дочерние узлы.

Можно было хранить формулу строкой и каждый раз разбирать заново, но это медленно. Ещё был вариант с обратной польской записью, но с ней сложно сделать `IF` и печать формулы обратно в текст.

### Visitor — операции над деревом формулы

С деревом нужно делать разные вещи: считать, печатать, искать ссылки, а на втором этапе ещё сдвигать ссылки. Если сделать для этого методы в каждом узле, то при каждой новой операции придётся менять все классы узлов. С посетителем новая операция — это просто новый класс.

Минус в том, что при добавлении нового вида узла придётся менять всех посетителей. Но виды узлов почти не меняются, а новые функции добавляются через `Function`, так что это не проблема.

### Фабрика — FunctionRegistry

Парсеру нужно создать функцию по её имени из формулы. Можно было написать `if (name == "SUM") ... else if ...`, но тогда для каждой новой функции пришлось бы менять парсер. Вместо этого есть реестр, где по имени лежит функция, создающая нужный объект.

`ActionRegistry` устроен так же: он создаёт действие по имени из конфига клавиш.

### Observer — уведомления об изменениях

Когда ячейки поменялись, интерфейсу нужно перерисоваться, но ядро ничего не должно знать про интерфейс. Поэтому `Spreadsheet` сообщает об изменениях своим подписчикам (это `Session`), а `Session` сообщает `TuiApp`. Так же интерфейс узнаёт, что закончилась фоновая загрузка файла.

Сначала я хотел подписывать ячейки друг на друга, но тогда некоторые ячейки пересчитывались бы по два раза, и циклы было бы трудно находить. Поэтому порядок пересчёта определяет граф зависимостей.

### Command — действия и отмена

Клавиши берутся из конфига, поэтому каждое действие сделано отдельным объектом с именем. Со `switch` по клавишам переназначить их из файла не получилось бы.

На втором этапе так же будет работать отмена: каждое изменение (`EditCommand`) умеет и выполниться, и откатиться. Сохранять перед каждым изменением копию всей таблицы заняло бы слишком много памяти.

### State — режимы

В программе есть несколько режимов: обычный, редактирование ячейки, ввод команды и справка. В каждом режиме клавиши работают по-своему. Если делать это через `switch` по режиму, одна функция получится огромной. Поэтому каждый режим — отдельный класс.

### Strategy — форматы файлов и чисел

Таблицу можно сохранить в разных форматах: в моём собственном и в CSV. Каждый формат — отдельный класс с одинаковым интерфейсом `FileFormat`. Иначе пришлось бы писать много методов `saveCsv`, `loadCsv` и т.д. прямо в `Spreadsheet`.

На втором этапе форматы чисел (`NumberFormat`) будут устроены так же.

### Facade — Spreadsheet

Чтобы записать значение в ячейку, нужно разобрать формулу, обновить граф, пересчитать ячейки и сообщить подписчикам. Всё это делает `Spreadsheet`, а `app` вызывает только `setInput`. Так проще следить, чтобы при ошибке таблица не испортилась.

### Другие

- Слои: `core`, `app` и интерфейс (описаны выше).
- MVC: модель — `Spreadsheet` и `Session`, контроллер — режимы и действия, вид — классы `*View`.
