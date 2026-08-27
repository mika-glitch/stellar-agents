vec3 a_position  : POSITION;
vec3 a_normal    : NORMAL;

// Hardware Instancing Matrix (M_model components)
vec4 i_data0     : TEXCOORD4;
vec4 i_data1     : TEXCOORD5;
vec4 i_data2     : TEXCOORD6;
vec4 i_data3     : TEXCOORD7;
// Instance Color Payload
vec4 i_data4     : COLOR0;

// Rasterizer Interpolators
vec4 v_color0    : COLOR0;
vec3 v_normal    : NORMAL;