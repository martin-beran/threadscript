# Print all command line arguments

seq(
    var("n", size(_cmdline())),
    print("argc=", n(), "\n"),
    var("i", 0),
    while(lt(i(), n()), seq(
        print("argv[", i(), "]=\"", at(_cmdline(), i()), "\"\n"),
        var("i", add(i(), 1))
    )),
    0
)
