add_lldb_library(lldbPluginABISw64 PLUGIN
  ABISw64.cpp
  ABISysV_sw_64.cpp
  ABISysV_sw_64.cpp

  LINK_LIBS
    lldbCore
    lldbSymbol
    lldbTarget
  LINK_COMPONENTS
    Support
    TargetParser
  )
