Import("env")
flags = ["-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"]
env.Append(CCFLAGS=flags, ASFLAGS=flags, LINKFLAGS=flags)
