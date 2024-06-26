#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Buffer Definitions: 
//
// cbuffer UBOBlock
// {
//
//   float4 u_src_rect;                 // Offset:    0 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// UBOBlock                          cbuffer      NA          NA            cb0      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_VertexID              0   x           0   VERTID    uint   x   
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
// SV_Position              0   xyzw        1      POS   float   xyzw
//
vs_4_0
dcl_constantbuffer CB0[1], immediateIndexed
dcl_input_sgv v0.x, vertex_id
dcl_output o0.xy
dcl_output_siv o1.xyzw, position
dcl_temps 1
ishl r0.x, v0.x, l(1)
and r0.x, r0.x, l(2)
and r0.z, v0.x, l(2)
utof r0.xy, r0.xzxx
mad o0.xy, r0.xyxx, cb0[0].zwzz, cb0[0].xyxx
mad o1.xy, r0.xyxx, l(2.000000, -2.000000, 0.000000, 0.000000), l(-1.000000, 1.000000, 0.000000, 0.000000)
mov o1.zw, l(0,0,0,1.000000)
ret 
// Approximately 8 instruction slots used
#endif

const BYTE static s_display_vs_bytecode[] = {
  68,  88,  66,  67,  37,  97,  157, 234, 112, 10,  38,  98,  114, 228, 143, 118, 71,  158, 122, 195, 1,   0,   0,
  0,   72,  3,   0,   0,   5,   0,   0,   0,   52,  0,   0,   0,   248, 0,   0,   0,   44,  1,   0,   0,   132, 1,
  0,   0,   204, 2,   0,   0,   82,  68,  69,  70,  188, 0,   0,   0,   1,   0,   0,   0,   72,  0,   0,   0,   1,
  0,   0,   0,   28,  0,   0,   0,   0,   4,   254, 255, 0,   129, 0,   0,   148, 0,   0,   0,   60,  0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,
  0,   1,   0,   0,   0,   85,  66,  79,  66,  108, 111, 99,  107, 0,   171, 171, 171, 60,  0,   0,   0,   1,   0,
  0,   0,   96,  0,   0,   0,   16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   120, 0,   0,   0,   0,
  0,   0,   0,   16,  0,   0,   0,   2,   0,   0,   0,   132, 0,   0,   0,   0,   0,   0,   0,   117, 95,  115, 114,
  99,  95,  114, 101, 99,  116, 0,   171, 1,   0,   3,   0,   1,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   77,  105, 99,  114, 111, 115, 111, 102, 116, 32,  40,  82,  41,  32,  72,  76,  83,  76,  32,  83,  104, 97,
  100, 101, 114, 32,  67,  111, 109, 112, 105, 108, 101, 114, 32,  49,  48,  46,  49,  0,   73,  83,  71,  78,  44,
  0,   0,   0,   1,   0,   0,   0,   8,   0,   0,   0,   32,  0,   0,   0,   0,   0,   0,   0,   6,   0,   0,   0,
  1,   0,   0,   0,   0,   0,   0,   0,   1,   1,   0,   0,   83,  86,  95,  86,  101, 114, 116, 101, 120, 73,  68,
  0,   79,  83,  71,  78,  80,  0,   0,   0,   2,   0,   0,   0,   8,   0,   0,   0,   56,  0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   12,  0,   0,   65,  0,   0,   0,   0,
  0,   0,   0,   1,   0,   0,   0,   3,   0,   0,   0,   1,   0,   0,   0,   15,  0,   0,   0,   84,  69,  88,  67,
  79,  79,  82,  68,  0,   83,  86,  95,  80,  111, 115, 105, 116, 105, 111, 110, 0,   171, 171, 171, 83,  72,  68,
  82,  64,  1,   0,   0,   64,  0,   1,   0,   80,  0,   0,   0,   89,  0,   0,   4,   70,  142, 32,  0,   0,   0,
  0,   0,   1,   0,   0,   0,   96,  0,   0,   4,   18,  16,  16,  0,   0,   0,   0,   0,   6,   0,   0,   0,   101,
  0,   0,   3,   50,  32,  16,  0,   0,   0,   0,   0,   103, 0,   0,   4,   242, 32,  16,  0,   1,   0,   0,   0,
  1,   0,   0,   0,   104, 0,   0,   2,   1,   0,   0,   0,   41,  0,   0,   7,   18,  0,   16,  0,   0,   0,   0,
  0,   10,  16,  16,  0,   0,   0,   0,   0,   1,   64,  0,   0,   1,   0,   0,   0,   1,   0,   0,   7,   18,  0,
  16,  0,   0,   0,   0,   0,   10,  0,   16,  0,   0,   0,   0,   0,   1,   64,  0,   0,   2,   0,   0,   0,   1,
  0,   0,   7,   66,  0,   16,  0,   0,   0,   0,   0,   10,  16,  16,  0,   0,   0,   0,   0,   1,   64,  0,   0,
  2,   0,   0,   0,   86,  0,   0,   5,   50,  0,   16,  0,   0,   0,   0,   0,   134, 0,   16,  0,   0,   0,   0,
  0,   50,  0,   0,   11,  50,  32,  16,  0,   0,   0,   0,   0,   70,  0,   16,  0,   0,   0,   0,   0,   230, 138,
  32,  0,   0,   0,   0,   0,   0,   0,   0,   0,   70,  128, 32,  0,   0,   0,   0,   0,   0,   0,   0,   0,   50,
  0,   0,   15,  50,  32,  16,  0,   1,   0,   0,   0,   70,  0,   16,  0,   0,   0,   0,   0,   2,   64,  0,   0,
  0,   0,   0,   64,  0,   0,   0,   192, 0,   0,   0,   0,   0,   0,   0,   0,   2,   64,  0,   0,   0,   0,   128,
  191, 0,   0,   128, 63,  0,   0,   0,   0,   0,   0,   0,   0,   54,  0,   0,   8,   194, 32,  16,  0,   1,   0,
  0,   0,   2,   64,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   128, 63,  62,
  0,   0,   1,   83,  84,  65,  84,  116, 0,   0,   0,   8,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
  3,   0,   0,   0,   2,   0,   0,   0,   1,   0,   0,   0,   2,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,
  0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

static const char* s_display_vs_shader =
  "cbuffer UBOBlock : register(b0)"
  "{"
  "float4 u_src_rect;"
  "};"
  "struct VertexIn {"
  "float2 pos : POSITION;"
  "};"
  "void main(in VertexIn input, out float2 v_tex0 : TEXCOORD0, out float4 o_pos : SV_Position)"
  "{"
  "float2 pos = input.pos;"
  "v_tex0 = u_src_rect.xy + pos * u_src_rect.zw;"
  "o_pos = float4(pos * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);"
  "}";