#!/bin/bash

gcc hypl.c hyperion/value.c hyperion/object.c hyperion/memory.c hyperion/HVM.c hyperion/chunk.c hyperion/debug.c hyperion/compiler.c hyperion/lexer.c hyperion/table.c hyperion/commandline.c hyperion/std/time_module/time.c hyperion/std/math_module/math.c hyperion/std/type_conversion_module/type_conversion.c hyperion/std/file_io_module/file_io.c hyperion/std/console_module/console.c hyperion/std/list_module/list.c hyperion/std/sys_module/sys.c hyperion/std/os_module/os.c hyperion/std/string_module/string.c hyperion/std/random_module/random.c  -o hypl