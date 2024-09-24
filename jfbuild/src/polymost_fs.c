// Generated by text2c.pl. Do not edit directly.

const char default_polymost_fs_glsl[] =
	"#glbuild(ES2) #version 100\n"
	"#glbuild(2)   #version 110\n"
	"#glbuild(3)   #version 140\n"
	"\n"
	"#ifdef GL_ES\n"
	"precision lowp float;\n"
	"#  define o_fragcolour gl_FragColor\n"
	"#elif __VERSION__ < 140\n"
	"#  define mediump\n"
	"#  define o_fragcolour gl_FragColor\n"
	"#else\n"
	"#  define varying in\n"
	"#  define texture2D texture\n"
	"out vec4 o_fragcolour;\n"
	"#endif\n"
	"\n"
	"uniform sampler2D u_texture;\n"
	"uniform sampler2D u_glowtexture;\n"
	"uniform vec4 u_colour;\n"
	"uniform float u_alphacut;\n"
	"uniform vec4 u_fogcolour;\n"
	"uniform float u_fogdensity;\n"
	"\n"
	"varying mediump vec2 v_texcoord;\n"
	"\n"
	"vec4 applyfog(vec4 inputcolour) {\n"
	"    const float LOG2_E = 1.442695;\n"
	"    float dist = gl_FragCoord.z / gl_FragCoord.w;\n"
	"    float densdist = u_fogdensity * dist;\n"
	"    float amount = 1.0 - clamp(exp2(-densdist * densdist * LOG2_E), 0.0, 1.0);\n"
	"    return mix(inputcolour, u_fogcolour, amount);\n"
	"}\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"    vec4 texcolour;\n"
	"    vec4 glowcolour;\n"
	"\n"
	"    texcolour = texture2D(u_texture, v_texcoord);\n"
	"    glowcolour = texture2D(u_glowtexture, v_texcoord);\n"
	"\n"
	"    if (texcolour.a < u_alphacut) {\n"
	"        discard;\n"
	"    }\n"
	"\n"
	"    texcolour = applyfog(texcolour);\n"
	"    o_fragcolour = mix(texcolour * u_colour, glowcolour, glowcolour.a);\n"
	"}\n"
;

const int default_polymost_fs_glsl_size = 1147;
