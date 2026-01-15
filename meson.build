project(
    'envi-lang',
    'cpp',
    version: '0.0.1',
    default_options: ['cpp_std=c++23'],
)

envi_src = [
    'lexer.cpp',
    'parser.cpp',
    'libraries/simdjson/simdjson.cpp'
]

executable('envi', 'envi.cpp',
    envi_src,
    install : true
)
