RooCeptor works by patching function prologues at runtime, redirecting execution to user-defined enter and leave callbacks. I wrote this back in early 2023 for my own security research. At the time, it was a personal prototype and wasn't intended for public release. Named after a bernedoodle who goes by the nickname "Auggieroonie."

One use-case I had for it was to encrypt static binaries to prevent sensitive functions from being easily reversed engineered using static analysis tools (i.e. IDA and Ghidra).
The high level idea was as followed:
1) Encrypt important functions in your binary.
2) Generate a custom debugger using RooCeptor that will hook the important encrypted functions.
3) Insert a stub at the prologue of each encrypted function, decrypting it.
4) Insert a stub at the epilogue of each encrypted function, re-encrypting it.

RooCeptor has other use cases too, such as using it as a lightweight debugger, tracing function arguments and return values, or experimenting with binary instrumentation techniques for educational purposes.

Build:

`armv7a-linux-androideabi17-clang++.cmd -fvisibility=hidden -shared -fPIC inject_example.cpp RooCeptor.cpp RooWriter.cpp RooMaps.cpp -o example.so`

This builds RooCeptor as a shared library that can be dynamically imported as followed:
`void *handle = dlopen("./example.so",0);`

This enables you to hook functions... here's an example inside of inject_example.cpp:

```
extern "C" void __attribute__((visibility("default"))) start() {
    address base = RooMaps::getModuleBase("a.out"); // or your target module name

    RooCeptor interceptor((void*)(base + 0x2C48)); // target function offset
    interceptor.attach((void*)&onEnter, (void*)&onLeave);
}
```

Then, call start after importing library:
```
    void (*start)() = (void (*)())dlsym(handle, "start");
    if (!start) {
        std::cerr << "dlsym(start) failed: " << dlerror() << std::endl;
        return 1;
    }
```
