solution("sockets_p3")

configurations({"Debug", "Release"})
flags({"NoRTTI"})

configuration("Release")
buildoptions({"-O3"})
linkoptions({"-O4"}) -- Link time optimization
flags({"StaticRuntime"})

configuration("Debug")
flags({"Symbols"})

configuration({})

project("sockets_part_3")
kind("ConsoleApp")
language("C++")
buildoptions({"-std=c++11"})
linkoptions({"-pthread"})
links({"ssl", "crypto"})
libdirs({"/usr/local/lib"})
files({"main.cpp"
       , "ssl_socket.cpp"
})

