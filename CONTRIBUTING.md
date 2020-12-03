OpenQL C++ Coding Conventions
=============================

In order to maintain the code homogeneous and consistent, all contibutors are
invited to follow this coding convention.

NOTE: at the time of writing, not all of OpenQL has been converted to this
code style completely yet.

## File and directory organization

C++ header files should be named `.h`. Header files private to OpenQL go in
the `src` directory, header files that a user needs to access as well (the vast
majority) go in `include`.

All definitions must go into source files; header files should only declare
things. Therefore, almost all header files need a corresponding source file.
This file must have the same name and path relative to the src/include
directory as the corresponding header file.

All filenames are lowercase, separated by `_` when composed of multiple words.

## Naming conventions

To be consistent with especially Python (since we share an API between it and
C++):

 - class and type names are written in `TitleCase`.
 - variables, fields, and namespaces (compare to modules) are written in
   `snake_case`.
 - constants and macros are written in `UPPER_CASE`.

This is already the standard in most popular languages (aside from some
deciding to use `mixedCase` in places). C++ is the only serious language
that remains that maintains sort of its own style, but it's also the one
largest amount of conflicting styles. Therefore, it makes more sense to just
stick to Python. The only annoying conflict is that the standard library
types are lowercase.

Since OpenQL is a library, it's important not to pollute the global namespace
with stuff. Imagine, for instance, if OpenQL would define the type `Bit` to
represent a classical bit in the global namespace, and someone using the library
from C++ also includes a bit manipulation library that happens to also define
`Bit`; this would be a naming conflict that's impossible to resolve for the
user. Therefore, everything defined by OpenQL should be in the `ql` namespace,
and all preprocessor macros (which can't be namespaced) should start with `QL_`.

## Utils types and functions

The `ql::utils` namespace provides a bunch of typedefs and wrappers for C++
standard library stuff, modified to improve safety, reduce undefined behavior,
simplify stuff where OpenQL doesn't need the full expressive power of the
standard library, improve consistency in terms of naming conventions, or just
to reduce typing. In cc files there is usually a `using namespace utils` to
reduce typing further (but don't do this in header files!). You should use
types and functions from here as much as possible. Here are the important ones.

 - From `utils/num.h`:
    - `Bool` for booleans;
    - `Byte` for (unsigned) bytes;
    - `UInt` for unsigned integers (maps to 64-bit unsigned);
    - `Int` for signed integers (maps to 64-bit signed);
    - `Real` for real numbers (maps to `double`);
    - `Complex` for complex numbers;
    - `MAX` as an "undefined" value for Int or UInt;
    - `PI` for pi;
    - `EU` for Euler's constant;
    - `IM` for the imaginary unit;
    - a bunch of common math subroutines from std are copied into utils, so
      you don't need to type `std::`.
 - From `utils/str.h`:
    - `Str` for strings (maps to `std::string`);
    - `StrStrm` for string streams (maps to `std::ostringstream`);
    - `to_string()` for converting things to string (this uses the << stream
      operator, so it can be overloaded, and IS overloaded for the wrapped
      container types for instance);
    - `parse_uint()`, `parse_int()`, and `parse_real()` for parsing numbers
      (these throw better exceptions than the STL counterparts);
    - a couple additional string utility functions are defined here.
 - From `utils/json.h`:
    - `Json` for JSON values (from `nlohmann::json`);
    - `load_json()` for loading JSON files with better contextual errors and
      allowance for `//` comments.
 - From `utils/pair.h`:
    - `Pair<A, B>` for `std::pair`; includes stream << overload.
 - From `utils/vec.h`:
    - `Vec<T>` for a wrapper of `std::vector<T>` with additional safety.
 - From `utils/list.h`:
    - `List<T>` for a wrapper of `std::list<T>` with additional safety.
 - From `utils/map.h`:
    - `Map<K, V>` for a wrapper of `std::map<K, V>` with additional safety.
 - From `utils/opt.h`:
    - `Opt<V>` for a more-or-less equivalent of `std::optional`, implemented
      using a smart pointer. Primarily intended for places where you need to
      own a value but can't construct it immediately (replacing the "virgin
      constructor" antipattern).
 - From `utils/tree.h`:
    - `Maybe<T>` for an optional tree node reference;
    - `One<T>` for a mandatory tree node reference;
    - `Any<T>` for zero or more tree node references;
    - `Many<T>` for one or more tree node references;
    - `OptLink<T>` for an optional link to a tree node elsewhere in a tree;
    - `Link<T>` for a mandatory link to a tree node elsewhere in a tree;
    - `make<T>()` for constructing tree nodes.
 - From `utils/exception.h`:
    - `Exception` for exceptions that are unlikely to be caught. This exception
      automatically adds contextual information, such as a stack trace.
 - From `utils/logger.h`:
    - macros for logging to stdout; these are preferred over streaming to
      `std::cout` directly.
 - From `utils/filesystem.h`:
    - `OutFile` for writing files (wraps `std::ofstream` with automatic
      error-checking, and also ensures directories are created recursively if
      the path to the file doesn't exist yet);
    - `InFile` for reading files (wraps `std::ifstream` with automatic
      error-checking);
    - a couple additional FS utilities.

## Indentation

Use four spaces. NEVER tabs. Avoid trailing whitespace.

## Comments

```cpp
// A good normal comments consists of two slashes at the front of each line,
// followed by a single space, followed by English text in the form of a
// paragraph, preferably manually wrapped at column 80.

// Comment blocks of code like this.
statement;
another statement;

// The next block of code starts after an empty line. The first two statements
// that follow relate to this comment, the third does not.
statement one;
if (condition) {
    statement two;
}

statement three;

// In indented blocks, it works like this.

// Comment A
belongs to A;
belongs to A;
if (condition) {
    belongs to A;

    // Comment B
    belongs to B

}
belongs to A;

// Avoid comments at the end of a line. This almost always just becomes way too
// wide to read easily.

// Avoid /*...*/ for descriptive comments. You can use them to disable code,
// but #if 0 blocks are preferable because you can nest those.

// Functions, variables, classes, etc. should use javadoc-style comments.
// These comments may then be used by Doxygen and IDEs to extract
// documentation. You can use markdown and Doxygen @directives to make the docs
// prettier if you like. Attach the docstring to the *definition* of the object
// rather than its declaration in a header file; if you like, you can add
// regular // comments with a brief description of the function or whatever in
// the header file for people trying to understand the interface of your class
// or module from a birds' eye perspective.
/**
 * Brief documentation.
 *
 * Extended documentation automatically follows after the first sentence.
 */
void some_function();

// When you want to section up your code, you can use breaks like this:

//=============================================================================
// START OF SOME SECTION
//=============================================================================

// Don't use "/*********" etc., as this may be confused with docstrings by some
// tools.
```

## Macros and other preprocessor directives

```cpp
// Regardless of code indentation, preprocessor directives are not indented by
// convention.
void test() {
    ...
#if WITH_SOME_FEATURE
    ...
#endif
    ...
}

// Feature flags are controlled by CMake through options passed to the user.
// Refer to the CMakeLists.txt files for more information; it should be fairly
// obvious.

// Feature switches such as the above must only ever be used in .cc files,
// never in headers. The feature flags may get out of sync otherwise (the flags
// may differ between when the OpenQL library was compiled and when the user is
// linking against it).

// Header file guards are done using
#pragma once
// rather than #ifndef etc. This avoids weird problems caused by copypaste
// mistakes, when the same preprocessor definition is used for multiple
// headers. Since OpenQL already requires C++11, you'll be hard-pressed finding
// a compiler that doesn't support that pragma but could compile OpenQL
// otherwise.

// Local header files use "" in their include directives. Only external
// libraries and the standard library uses <>.
#include <vector>
#include <lemon/...>
#include "openql.h"
```

## If statements

```cpp
// If statements look like this.
if (condition) {
    ...
} else if (condition) {
    ...
} else {
    ...
}

// Very short statements can go on the same line without braces, but only if
// there is no else block and it's easier to read than writing out the block.
// Typically this is only the case for if-break, if-return, or if-throw.
if (condition) break;

// Do NOT use Yoda conditions. It's harder to read, and unless everything is
// written that way it's inconsistent.

// When the condition becomes too long to be readable, indent as shown:
if (
    (x == "a")
    || (x == "b")
    || (x == "c")
) {
    ...
}

```

## Switch statements

```cpp
// Switch statements:
//  - Indent case labels.
//  - Explicitly mark fallthrough with a comment when intended.
//  - Use blocks only if needed (i.e. for variable declarations).
switch (condition) {
    case a:
        ...
        break;
    case b:
    case c: {
        ...
        break;
    }
    case d:
        ...
        // fallthrough
    case e:
        ...
        break;
    default:
        break;
}
```

## Loops

```cpp
// Normal for loops:
//  - Declare the loop variable in the loop if it's a local thing.
//  - Use size_t for loops instead of int whenever you're using the loop
//    variable as index to avoid signed-unsigned comparison warnings.
//  - i++ is preferential over ++i unless you have a good reason to believe
//    the additional sequence point will actually cause harm. It's C++ after
//    all, not ++C.
for (size_t i = 0; i < 10; i++) {
    ...
}

// Iterating:
//  - Use iterating loops whenever possible.
//  - Use const auto &x whenever possible. If you intend to mutate the
//    elements, drop the const; if you don't intend to mutate but get
//    errors, drop the const and &.
for (const auto &element : sequence) {
    ...
}

// While loops:
while (condition) {
    ...
}

// Do-while loops (if you need them...):
do {
    ...
} while (condition);

// ALWAYS open a block (this goes for all statements except exceptional
// if statements). In rare cases (when the loop is just annoying boilerplate)
// the block can go in a single line as follows:
for (auto &el : sequence) { el = 0; }
```

## Namespaces

```cpp
// Namespaces do not receive indentation, because then everything would be
// indented. But the closing brace must be clearly marked as such (using the
// depicted style) to compensate.
namespace ql {

... // namespace contents are not indented...

} // namespace ql

// "using namespace" is allowed only in .cc files, NEVER in header files, and
// CERTAINLY NEVER in the global namespace; a single "using namespace" in any
// header completely destroys the idea behind namespaces (that is, preventing
// name conflicts or long names with many redundant parts in them).
```

## Enumerations

```cpp
// Enumerations look like this:
/**
 * Documentation for the complete enumeration.
 */
enum class Enum {

    /**
     * Documentation for option A.
     */
    OPTION_A,

    /**
     * Documentation for option B.
     */
    OPTION_B,

    /**
     * Documentation for option C.
     */
    OPTION_C

};

// Using the C++ "enum class" construct, you can use short names for your enum
// options, as they will be namespaced to the enum;
Enum::OPTION_A
```

## Typedefs

```cpp
// Use "using" rather than "typedef". The syntax is just more clear.
using Hello = std::vector<int>*;

// instead of "typedef std::vector<int> *Hello"
```

## Classes

```cpp
// Class definitions are indented and whitespaced as follows:
class AClass {
public:
    ...
}

// Classes in header files should not contain any function definitions,
// regardless of triviality (except when templated, then it can't be avoided).
// This keeps compilation times down, the lack of inlining makes stack traces
// less magical, and honestly, I doubt you'll notice the performance penalty of
// the lack of inlining in practice (and there's always LTO nowadays).

// AVOID "virgin constructors": the constructor of a class must also initialize
// that class. C++ utilizes the RAII principle (resource acquisition is
// initialization): if you have a class instance, you may assume it has been
// initialized. These virgin constructors combined with an init method nuke
// this principle and result in the worst of both worlds; you still have a
// complaining compiler, but don't have any of the benefits. They simply should
// not exist.

// Constructions where you think you need this should be avoided. If you really
// need it anyway, use ql::Opt<T> instead of T directly. You can then use
// emplace(...) to initialize the contained object of type T after the fact
// (or you can just assign it as you otherwise would), and access the fields
// and members of T using -> instead of . (so, as if it were a pointer). The
// practical upshot of this is that Opt will throw an exception your way if
// you try to access it while it's not initialized yet, whereas with the virgin
// constructor method OpenQL would just silently truck along to spit out
// garbage that may or may not look like what you were expecting.

// When using inheritance, the toplevel ancestor class must have a virtual
// destructor. If you don't have anything useful to put there, you can tell the
// compiler to make a default one. If you don't do this, class destruction
// won't work right, and you'll get undefined behavior. It is sufficient to
// only give the toplevel class such a virtual destructor; all descendents get
// virtual destructors automatically because of it.
virtual ~Cls() = default;
```

## Variables and fields

```cpp
// References and pointers belong to the name, not to the type:
int *ip;

// so, NOT "int* ip". This is objectively the "correct" way to write this, as
// this is how C++'s syntax tree works. When you write "int* a, b", the result
// will unfortunately be that a is of type int* and b is of type int; to make
// them both int* you need to write "int *a, *b". This is complete nonsense,
// but unfortunately what the C++ overlords decided to go with.

// Public fields are almost always a Bad Idea (TM). Whenever the outside world
// should probably not have write access to a field (variable in a class)
// because this would break things unless a large amount of care is taken, the
// field must be private or protected (depending on whether descendents are
// part of the "outside world" or not). When it makes sense, you can add
// getters (make sure these are const-correct) and/or setters (when
// applicable). Debugging is NOT a good reason to make things public; use
// getters!

// Shared global variables need to be declared as follows in a header file:
OPENQL_DECLSPEC extern int i;

// and as follows in the corresponding source file:
int i;

// OPENQL_DECLSPEC is defined in utils.h. If you don't do this, the OpenQL
// library will break on Windows.
```

## Functions and methods

```cpp
// Declaration in header file:

// A simple function.
int name(int a, int b = 2, const std::string &s = "hello");

// Implementation in source file:

/**
 * A simple function.
 *
 * All functions should be documented with javadoc-style comments like these.
 */
int name(int a, int b = 2, const std::string &s) {
    ...
}

// If the parameter list becomes annoyingly long, indent as follows:
int name(
    int a,
    int b,
    int c,
    int d
) {
    ...
}

// Constructor lists either go on a single line along with all parameters:
int name(int a, int b) : a(a), b(b) {
    ...
}

// or look like this:
int name(
    int a,
    int b
) :
    a(a),
    b(b)
{
    ...
}

// Template statements go on the line in front of the function. Templated
// functions belong in header files, unless you really know what you're
// doing (explicitly instantiated specializations, etc.).
template <class A>
int *name(int a, const std::string &s) {
    ...
}

// Methods (functions on a class) should be static if they don't use any
// fields,
class Cls {
    ...
    static void func();
    ...
};

void Cls::func() {
    ...
}

// or const if they only read fields:
class Cls {
    ...
    void func() const;
    ...
};

void Cls::func() const {
    ...
}

// If you're intending to override methods, the method in the ancestor class
// should be marked virtual:
class Cls {
    ...
    virtual void func();
    ...
}

void Cls::func() {
    ...
}

// and the overrides should be marked with override:
class Cls {
    ...
    void func() override;
    ...
}

void ClsImpl::func() {
    ...
}

// Note that static, virtual, and override don't appear in the implementation
// for some reason, and the position of the statements is arbitrary at best.
// This is just how C++ works, unfortunately.

// Pass nontrivial argument types (anything that's larger than a pointer) by
// const reference unless you have a good reason not to. That is, when passing
// some random string, it should be done like
void print(const std::string &str);

// If you really know what you're doing you can deviate (rvalue references,
// perfect forwarding, etc.), but this is the norm.

// Passing mutable references for the purpose of mutating them in the function
// is allowed, but MUST BE CLEARLY DOCUMENTED.
/**
 * Mutate the given string to make it lowercase.
 */
void to_lower(std::string &str);

// Passing by raw pointer is frowned upon, but still happens all over at the
// time of writing. At least be const-correct about it if you can.
```

## Expressions and function calls

```cpp
// Apply whitespace and interpunction rules as if you were writing English,
// except for the open-parenthesis that immediately follows a function name,
// where the space is omitted. For example, don't write something like
// "hello ( a+b , ( x+y )*z )", but write
hello(a + b, (x + y) * z);

// When expressions are too long to fit in a single line, preferably indent as
// follows if you have parentheses and a comma-separated list:
hello(
    a + b,
    (x + y) * z
);

// If you don't have anything like that, you might want to introduce
// parentheses and wrap as follows:
long_expression * (
    long_expression + long_expression
) * long_expression

// but in general, do whatever looks right (within context).
```
