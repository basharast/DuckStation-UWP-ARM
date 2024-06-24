host_build {
    QT_CPU_FEATURES.x86_64 = sse sse2
} else {
    QT_CPU_FEATURES.arm64 = 
}
QT.global_private.enabled_features = alloca_malloc_h alloca dbus gui network sql testlib widgets xml relocatable cross_compile largefile precompile_header
QT.global_private.disabled_features = android-style-assets gc_binaries private_tests reduce_exports reduce_relocations sse2 sse3 ssse3 sse4_1 sse4_2 avx f16c avx2 avx512f avx512er avx512cd avx512pf avx512dq avx512bw avx512vl avx512ifma avx512vbmi aesni rdrnd rdseed shani mips_dsp mips_dspr2 neon posix_fallocate alloca_h stack-protector-strong system-zlib zstd dbus-linked libudev dlopen intelcet
CONFIG += cross_compile largefile precompile_header
QT_COORD_TYPE = double
QT_BUILD_PARTS = libs

