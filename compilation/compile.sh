#!/bin/bash

g++ hypl.cpp hyperion/value.c hyperion/object.c hyperion/memory.c hyperion/HVM.c hyperion/chunk.c hyperion/debug.c hyperion/compiler.c hyperion/lexer.c hyperion/table.c hyperion/std/time_module/time_module.c -o hypl