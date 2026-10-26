{
  "targets": [
    {
      "target_name": "dwa_node",
      "sources": [
        "dwa_node.cc",
        "dwa.c",
        "dwa_json.c",
        "dwa_op.c",
        "dwa_validate.c",
        "../co/co.c",
        "../co/co_dnf.c",
        "../co/co_bitset.c"
      ],
      "include_dirs": [
        ".",
        "../co",
        "../co/expat",
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "XML_GE=1"
      ],
      "cflags": [
        "-msse4.2"
      ],
      "cflags_cc": [
        "-std=c++17",
        "-msse4.2"
      ],
      "xcode_settings": {
        "OTHER_CPLUSPLUSFLAGS": ["-std=c++17"]
      }
    }
  ]
}
