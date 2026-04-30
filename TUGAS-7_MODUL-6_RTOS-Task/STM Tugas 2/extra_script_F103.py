"""
Custom PlatformIO build script for STM32CubeF1 + FreeRTOS
Adds FreeRTOS include paths and compiles FreeRTOS kernel sources.
"""
Import("env")
import os

platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-stm32cubef1")

framework_includes = []

def add_include_path(base_path, relative_paths):
    for rel_path in relative_paths:
        full_path = os.path.join(base_path, rel_path)
        if os.path.exists(full_path):
            framework_includes.append(full_path)

add_include_path(FRAMEWORK_DIR, [
    "Drivers/STM32F1xx_HAL_Driver/Inc",
    "Drivers/CMSIS/Device/ST/STM32F1xx/Include",
    "Drivers/CMSIS/Include",
])

freertos_base = os.path.join(
    FRAMEWORK_DIR, "Middlewares", "Third_Party", "FreeRTOS", "Source"
)
add_include_path(freertos_base, [
    "include",
    "portable/GCC/ARM_CM3",
])

env.Append(CPPPATH=framework_includes)

freertos_sources = [
    "tasks.c", "queue.c", "list.c", "timers.c",
    "portable/GCC/ARM_CM3/port.c",
    "portable/MemMang/heap_4.c",
]

freertos_objs = []
for src in freertos_sources:
    src_path = os.path.join(freertos_base, src)
    if os.path.exists(src_path):
        obj_name = src.replace("/", "_").replace(".c", ".o")
        obj_path = os.path.join("$BUILD_DIR", "FreeRTOS_obj", obj_name)
        obj = env.Object(obj_path, src_path)
        freertos_objs.append(obj)

env.Append(PIOBUILDFILES=freertos_objs)