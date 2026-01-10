#!/usr/bin/env python3

import sys
import os
from pathlib import Path
import subprocess
import multiprocessing
import shutil

DEFAULT_TARGET = "all"
DEFAULT_PROFILE = "release"
DEFAULT_TOOLCHAIN = "gcc"
INITRD_NAME = "initrd.tar"

targets = []
arch = None
params = []
profile = None
toolchain = None

all_targets = ("tools", "kernel", "export", "drivers", "libs", "base", "initrd")
target_vals = all_targets + ("all", "doc", "configure", "clean")
clean_targets = ("tools", "kernel", "drivers", "libs", "base")
arch_vals = ("i686", )
params_vals = {"i686": ("smp", "pae")}
profile_vals = ("debug", "release")
toolchain_vals = ("gcc", )

kernel_params = {"smp": "-DSMP=1", "pae": "-DPAE=1"}

target_desc = {"tools": "Build host-side tools", 
               "kernel": "Build Nabla kernel", 
               "export": "Export kernel API and syscall API",
               "drivers": "Build all kernel mode drivers",
               "libs": "Build all user mode libraries",
               "base": "Build all fundamental Nabla programs and utilities",
               "initrd": "Prepare initial ramdisk",
               "doc": "Generate documentation",
               "all": "Build all - " + str(all_targets) + " in this order"
}
target_requires_arch = {"tools": False, "kernel": True, "export": False, "drivers": True, 
    "libs": True, "base": True, "initrd": False, "doc": False, "all": True, "configure": True, "clean": False}
build_path = {"tools": "tools/build", "kernel": "kernel/build", "drivers": "drivers/build", 
    "libs": "libs/build", "base": "base/build", "initrd": "initrd"}
cmake_generator = {"gcc": "Unix Makefiles"}

def execute(cmd):
    print("Executing " + subprocess.list2cmdline(cmd))
    result = subprocess.run(cmd)
    if result.returncode != 0:
        sys.exit("Command " + subprocess.list2cmdline(cmd) + " failed with status code " + str(result.returncode))
        

def make_target(target):
    print("Starting target " + target)
    cwd = os.getcwd()
    if target == "configure":
        path = cwd + "/" + build_path["tools"]
        Path(path).mkdir(parents = False, exist_ok = True)
        os.chdir(path)
        execute(["cmake", ".."])

        path = cwd + "/" + build_path["kernel"]
        Path(path).mkdir(parents = False, exist_ok = True)
        os.chdir(path)
        cmd = ["cmake", "..", "-G", cmake_generator[toolchain], "--toolchain=../{0}-{1}.cmake".format(toolchain, arch)]
        if profile == "debug":
            cmd.append("-DCMAKE_BUILD_TYPE=Debug")
        else:
            cmd.append("-DCMAKE_BUILD_TYPE=Release")
        for param in params:
            cmd.append(kernel_params[param])
        execute(cmd)

        path = cwd + "/" + build_path["drivers"]
        Path(path).mkdir(parents = False, exist_ok = True)
        os.chdir(path)
        cmd = ["cmake", "..", "-G", cmake_generator[toolchain], "--toolchain=../{0}-{1}.cmake".format(toolchain, arch)]
        if profile == "debug":
            cmd.append("-DCMAKE_BUILD_TYPE=Debug")
        else:
            cmd.append("-DCMAKE_BUILD_TYPE=Release")
        execute(cmd)
    elif target == "clean":
        for t in clean_targets:
            if(os.path.isdir(cwd + "/" + build_path[t])):
                shutil.rmtree(cwd + "/" + build_path[t])
    elif target == "kernel" or target == "tools":
        os.chdir(cwd + "/" + build_path[target])
        execute(["cmake", "--build", ".", "-j" + str(multiprocessing.cpu_count())])
    elif target == "export":
        execute([sys.executable, "tools/export.py"])
    elif target == "initrd":
        os.chdir(cwd + "/" + build_path["initrd"])
        execute(["tar", "-cf", cwd + "/image/" + INITRD_NAME] + os.listdir())
    elif target == "drivers":
        os.chdir(cwd + "/" + build_path["drivers"])
        execute(["cmake", "--build", ".", "--target=" + arch + "_drivers", "-j" + str(multiprocessing.cpu_count())])
    elif target == "libs":
        pass
    elif target == "base":
        pass
    elif target == "doc":
        execute(["doxygen", "doc/Doxyfile"])
    elif target == "all":
        for t in all_targets:
            make_target(t)
    else:
        sys.exit("Error: target " + target + " is unknown")
        
    print("Target " + target + " finished")
    os.chdir(cwd)

if len(sys.argv) == 1:
    print("Architecture name must be provided. "
          "Target defaults to {0}, profile defaults to {1}.\n".format(DEFAULT_TARGET, DEFAULT_PROFILE))

    print("Available architectures:")
    for a in arch_vals:
        print("{0} (options: {1})".format(a, str(params_vals[a])))
    print("\nAvailable profiles:")
    for p in profile_vals:
        print(p)
    print("\nAvailable targets:")
    for t in target_vals:
        print("{0} - {1}".format(t, target_desc[t]))
    sys.exit()

for arg in sys.argv[1:]:
    arg = arg.lower()
    if arg in target_vals:
        targets.append(arg)
    elif arg in arch_vals:
        if arch != None:
            sys.exit("Error: architecture already set to " + arch)
        else:
            arch = arg
    elif arg in profile_vals:
        if profile != None:
            sys.exit("Error: profile already set to " + profile)
        else:
            profile = arg
    elif arg in toolchain_vals:
        if toolchain != None:
            sys.exit("Error: toolchain already set to " + toolchain)
        else:
            toolchain = arg
    else:
        params.append(arg)

if not targets:
    targets.append(DEFAULT_TARGET)
if profile == None:
    profile = DEFAULT_PROFILE
if toolchain == None:
    toolchain = DEFAULT_TOOLCHAIN

if arch == None:
    for t in targets:
        if target_requires_arch[t]:
            sys.exit("Error: target {} requires architecture to be selected".format(t))
for param in params:
    if param not in params_vals[arch]:
        sys.exit("Error: parameter " + param + " unknown for " + arch)


print("Proceeding with " + str(len(targets)) + " " + profile + " targets " + str(targets) 
    + " for " + (arch, "n/a")[arch == None] + "-" + toolchain)
if params:
    print("Additional parameters: " + str(params))

for target in targets:
    make_target(target)





    