add_lldb_library(lldbPluginArchitectureSw64 PLUGIN
  ArchitectureSw64.cpp

  LINK_LIBS
    lldbCore
    lldbTarget
    lldbUtility
  LINK_COMPONENTS
    Support
  )
