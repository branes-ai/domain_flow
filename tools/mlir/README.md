# MLIR file readers

In this directory, there are the early experiments with mlir file readers.

The problem we discovered is that the parseSourceFile and readBytecodeFile
depend on the dialect processors to provide the IR parsing functionality.

Conclusion: readers that do not register any dialects have no ability to consume mlir files.

## Demonstration

If we have this `builtin.mlir` file in this directory:
```mlir
"builtin.module"() ( {
  %results:4 = "dialect.op1"() {"attribute name" = 42 : i32} : () -> (i1, i16, i32, i64)
  "dialect.op2"() ( {
    "dialect.innerop1"(%results#0, %results#1) : (i1, i16) -> ()
  },  {
    "dialect.innerop2"() : () -> ()
    "dialect.innerop3"(%results#0, %results#2, %results#3)[^bb1, ^bb2] : (i1, i32, i64) -> ()
  ^bb1(%1: i32):  // pred: ^bb0
    "dialect.innerop4"() : () -> ()
    "dialect.innerop5"() : () -> ()
  ^bb2(%2: i64):  // pred: ^bb0
    "dialect.innerop6"() : () -> ()
    "dialect.innerop7"() : () -> ()
  }) {"other attribute" = 42 : i64} : () -> ()
}) : () -> ()
```
When we try to read that with these mlir readers that have no dialects, 
we see this behavior:

```bash
tomtz@sw-desktop-300 MINGW64 /F/Users/tomtz/dev/branes/clones/dfa-dynamics/tools/mlir (main)
$ ../../build_msvc/VS17-Debug/tools/mlir/Debug/mlir_serialization.exe builtin.mlir
loc("builtin.mlir":2:29): error: operation being parsed with an unregistered dialect. If this is intended, please use -allow-unregistered-dialect with the MLIR tool used
Failed to parse MLIR module

tomtz@sw-desktop-300 MINGW64 /F/Users/tomtz/dev/branes/clones/dfa-dynamics/tools/mlir (main)
$ ../../build_msvc/VS17-Debug/tools/mlir/Debug/mlir_serialization.exe -allow-unregistered-dialect builtin.mlir
mlir_serialization.exe: Unknown command line argument '-allow-unregistered-dialect'.  Try: 'F:\Users\tomtz\dev\branes\clones\dfa-dynamics\build_msvc\VS17-Debug\tools\mlir\Debug\mlir_serialization.exe --help'
mlir_serialization.exe: Did you mean '--help-list-hidden'?
```

When we try to read that with the regular `opt` tools that do have registered dialects
we see this behavior:

```bash
tomtz@sw-desktop-300 MINGW64 /F/Users/tomtz/dev/branes/clones/dfa-dynamics/tools/mlir (main)
$ mlir-opt builtin.mlir
builtin.mlir:2:29: error: operation being parsed with an unregistered dialect. If this is intended, please use -allow-unregistered-dialect with the MLIR tool used
  %results:4 = "dialect.op1"() {"attribute name" = 42 : i32} : () -> (i1, i16, i32, i64)
                            ^

tomtz@sw-desktop-300 MINGW64 /F/Users/tomtz/dev/branes/clones/dfa-dynamics/tools/mlir (main)
$ ../../build_msvc/VS17-Debug/bin/dfa-opt.exe builtin.mlir
Current path is "F:\\Users\\tomtz\\dev\\branes\\clones\\dfa-dynamics\\tools\\mlir"
builtin.mlir:2:29: error: operation being parsed with an unregistered dialect. If this is intended, please use -allow-unregistered-dialect with the MLIR tool used
  %results:4 = "dialect.op1"() {"attribute name" = 42 : i32} : () -> (i1, i16, i32, i64)
                            ^

tomtz@sw-desktop-300 MINGW64 /F/Users/tomtz/dev/branes/clones/dfa-dynamics/tools/mlir (main)
$ mlir-opt --allow-unregistered-dialect builtin.mlir
module {
  %0:4 = "dialect.op1"() {"attribute name" = 42 : i32} : () -> (i1, i16, i32, i64)
  "dialect.op2"() ({
    "dialect.innerop1"(%0#0, %0#1) : (i1, i16) -> ()
  }, {
    "dialect.innerop2"() : () -> ()
    "dialect.innerop3"(%0#0, %0#2, %0#3)[^bb1, ^bb2] : (i1, i32, i64) -> ()
  ^bb1(%1: i32):  // pred: ^bb0
    "dialect.innerop4"() : () -> ()
    "dialect.innerop5"() : () -> ()
  ^bb2(%2: i64):  // pred: ^bb0
    "dialect.innerop6"() : () -> ()
    "dialect.innerop7"() : () -> ()
  }) {"other attribute" = 42 : i64} : () -> ()
}


tomtz@sw-desktop-300 MINGW64 /F/Users/tomtz/dev/branes/clones/dfa-dynamics/tools/mlir (main)
$ ../../build_msvc/VS17-Debug/bin/dfa-opt.exe -allow-unregistered-dialect builtin.mlir
Current path is "F:\\Users\\tomtz\\dev\\branes\\clones\\dfa-dynamics\\tools\\mlir"
module {
  %0:4 = "dialect.op1"() {"attribute name" = 42 : i32} : () -> (i1, i16, i32, i64)
  "dialect.op2"() ({
    "dialect.innerop1"(%0#0, %0#1) : (i1, i16) -> ()
  }, {
    "dialect.innerop2"() : () -> ()
    "dialect.innerop3"(%0#0, %0#2, %0#3)[^bb1, ^bb2] : (i1, i32, i64) -> ()
  ^bb1(%1: i32):  // pred: ^bb0
    "dialect.innerop4"() : () -> ()
    "dialect.innerop5"() : () -> ()
  ^bb2(%2: i64):  // pred: ^bb0
    "dialect.innerop6"() : () -> ()
    "dialect.innerop7"() : () -> ()
  }) {"other attribute" = 42 : i64} : () -> ()
}
```


Repeating the conclusion: mlir file readers that are not registering any dialects
do not have the ability to transform mlir files into an in-memory IR.

Any mlir reader used for working with an IR will need to follow the `opt` tool architecture
of registering dialects and using PassManagers to walk the IR.


Once you have that knowledge, this [documentation page](https://mlir.llvm.org/docs/Tutorials/UnderstandingTheIRStructure/) will make a lot more sense.
