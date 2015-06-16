#include "shaders.h"

#include <math.h>
#define xstr(a) str(a)
#define str(a) #a

/*
"#version 330\n"
"layout(triangles)in;"
"layout(triangle_strip,max_vertices=3)out;"
"void main(){"
    "gl_Position=gl_in[0].gl_Position;"
    "EmitVertex();"

    "gl_Position=gl_in[1].gl_Position;"
    "EmitVertex();"

    "gl_Position=gl_in[2].gl_Position;"
    "EmitVertex();"

    "EndPrimitive();"
"}"
*/

const shader_text shader[] = {
{ /* post processing */

"#version 330\n"
"layout(location=0)in vec2 pos;"
"out vec2 x;"
"void main(){"
    "x=(pos+vec2(1.0,1.0))*0.5;"
    "gl_Position=vec4(pos,0.0,1.0);"
"}",

0,

"#version 330\n"
"uniform sampler2D samp, k;"
"uniform vec2 matrix;"
"in vec2 x;"
"out vec4 c;"
"void main(){"
    "vec3 color = texture2D(samp, x).xyz;"
    "float m = texture2D(k, x).a;"
    "if (m == 0.0) {"
        "vec4 a = texture2D(k, x + vec2(matrix.x, 0.0)) * 0.25;"
        "a += texture2D(k, x + vec2(-matrix.x, 0.0)) * 0.25;"
        "a += texture2D(k, x + vec2(0.0, matrix.y)) * 0.25;"
        "a += texture2D(k, x + vec2(0.0, -matrix.y)) * 0.25;"
        "a += texture2D(k, x + vec2(-matrix.x, -matrix.y)) * 0.25;"
        "a += texture2D(k, x + vec2(matrix.x, -matrix.y)) * 0.25;"
        "a += texture2D(k, x + vec2(matrix.x, matrix.y)) * 0.25;"
        "a += texture2D(k, x + vec2(-matrix.x, matrix.y)) * 0.25;"

        "a = min(a,vec4(1.0,1.0,1.0,1.0));"
        "color = color * (1.0 - a.a) + a.rgb * a.a;"
    "}"
    "c=vec4(color,1.0);"
"}"
},
{ /* 2d */
"#version 330\n"
"layout(location=0)in vec4 p;"
"layout(location=1)in uint t;"
"layout(location=2)in vec4 a;"
"out vec4 pos;"
"out uint tex;"
"out vec4 col;"
"void main(){"
    "pos=p;"
    "tex=t;"
    "col=a;"
"}",
"#version 330\n"
"layout(points)in;"
"layout(triangle_strip,max_vertices=4)out;"
"uniform vec2 matrix;"
"in vec4 pos[];"
"in uint tex[];"
"in vec4 col[];"
"out vec2 t;"
"flat out uint z;"
"flat out vec4 a;"
"void main(){"
    "float tx=float(tex[0]&0xFFFu);"
    "float ty=float((tex[0]>>12u)&0xFFFu);"
    "float u=float((tex[0]>>24u)&0x1Fu)/8.0;"
    "vec4 p=pos[0];"
    "vec4 x=vec4(tx,ty,vec2(tx,ty)+p.zw)*(1.0/1024.0);"
    "p=vec4(p.xy,p.xy+p.zw*u);"

    "a=col[0];"
    "z=(tex[0]>>30u);"

    "t=x.xy;"
    "gl_Position=vec4(matrix*p.xy+vec2(-1.0,1.0),0.0,1.0);"
    "EmitVertex();"

    "t=x.xw;"
    "gl_Position=vec4(matrix*p.xw+vec2(-1.0,1.0),0.0,1.0);"
    "EmitVertex();"

    "t=x.zy;"
    "gl_Position=vec4(matrix*p.zy+vec2(-1.0,1.0),0.0,1.0);"
    "EmitVertex();"

    "t=x.zw;"
    "gl_Position=vec4(matrix*p.zw+vec2(-1.0,1.0),0.0,1.0);"
    "EmitVertex();"

    "EndPrimitive();"
"}",

"#version 330\n"
"uniform sampler2D zero, samp, k;"
"in vec2 t;"
"flat in uint z;"
"flat in vec4 a;"
"out vec4 c;"
"void main(){"
    "if(z==0u){c=a;}"
    "else if(z==1u){c=texture2D(zero,t)*a;}"
    "else if(z==2u){c=texture2D(samp,t)*a;}"
    "else{c=texture2D(k,t)*a;}"
"}"
},
{ /* map */
"#version 330\n"
"layout(location=0)in uint d;"
"layout(location=1)in vec4 h;"
"out uint data;"
"out vec4 height;"
"void main(){"
    "data=d;"
    "height=h;"
"}",

"#version 330\n"
"layout(points)in;"
"layout(triangle_strip,max_vertices=4)out;"
"uniform mat4 matrix;"
"in uint data[];"
"in vec4 height[];"
"out vec2 t;"
"void main(){"
    "float x=float((data[0]&0xFFFu)<<4u);"
    "float y=float((data[0]>>8u)&0xFFF0u);"
    "float u=float((data[0]>>24u)&15u)/16.0;"
    "float v=float(data[0]>>28u)/16.0;"

    "t=vec2(u,v);"
    "gl_Position=matrix*vec4(x,y,height[0].x,1.0);"
    "EmitVertex();"

    "t=vec2(u+(1.0/16.0),v);"
    "gl_Position=matrix*vec4(x+16.0,y,height[0].y,1.0);"
    "EmitVertex();"

    "t=vec2(u,v+(1.0/16.0));"
    "gl_Position=matrix*vec4(x,y+16.0,height[0].z,1.0);"
    "EmitVertex();"

    "t=vec2(u+(1.0/16.0),v+(1.0/16.0));"
    "gl_Position=matrix*vec4(x+16.0,y+16.0,height[0].w,1.0);"
    "EmitVertex();"

    "EndPrimitive();"
"}",

"#version 330\n"
"uniform sampler2D samp;"
"in vec2 t;"
"out vec4 c;"
"void main(){"
    "c=texture2D(samp,t);"
"}"
},
{ /* mapcicle */
"#version 330\n"
"layout(location=0)in uint d;"
"layout(location=1)in vec4 h;"
"layout(location=2)in vec2 o;"
"out uint data;"
"out vec4 height;"
"out vec2 origin;"
"void main(){"
    "data=d;"
    "height=h;"
    "origin=o;"
"}",

"#version 330\n"
"layout(points)in;"
"layout(triangle_strip,max_vertices=4)out;"
"uniform mat4 matrix;"
"in uint data[];"
"in vec4 height[];"
"in vec2 origin[];"
"out vec4 t;"
"void main(){"
    "float x=float((data[0]&0xFFFu)<<4u);"
    "float y=float((data[0]>>8u)&0xFFF0u);"
    "float r=float((data[0]>>24u));"
    "vec2 p=vec2(r*r,1.0/r);"

    "t=vec4(origin[0],p);"
    "gl_Position=matrix*vec4(x,y,height[0].x,1.0);"
    "EmitVertex();"

    "t=vec4(origin[0]+vec2(16.0,0.0),p);"
    "gl_Position=matrix*vec4(x+16.0,y,height[0].y,1.0);"
    "EmitVertex();"

    "t=vec4(origin[0]+vec2(0.0,16.0),p);"
    "gl_Position=matrix*vec4(x,y+16.0,height[0].z,1.0);"
    "EmitVertex();"

    "t=vec4(origin[0]+vec2(16.0,16.0),p);"
    "gl_Position=matrix*vec4(x+16.0,y+16.0,height[0].w,1.0);"
    "EmitVertex();"

    "EndPrimitive();"
"}",

"#version 330\n"
"in vec4 t;"
"out vec4 c;"
"void main(){"
    "c=vec4(0.0,1.0,0.0,1.0-abs(dot(t.xy,t.xy)-t.z)*t.w);"
"}"
},
{ /* model */
"#version 330\n"
"uniform mat4 matrix;"
"uniform vec4 r[62];"
"uniform vec3 d[62];"
"layout(location=0)in vec3 pos;"
"layout(location=1)in vec2 tex;"
"layout(location=2)in vec4 n;"
"layout(location=3)in uvec4 g;"
"out vec2 x;"

"vec3 qrot(vec4 q,vec3 v){return v+2.0*cross(q.xyz,cross(q.xyz,v)+q.w*v);}"

"void main(){"
    "x=tex/65536.0;"
    "float w=1.0/n.w;"
    "vec3 v = (qrot(r[g.x], pos) + d[g.x]) * w;"

    "int ng=int(n.w);"
    "if(ng>1){"
        "v += (qrot(r[g.y], pos) + d[g.y]) *  w;"
        "if(ng>2){"
            "v += (qrot(r[g.z], pos) + d[g.z]) * w;"
            "if(ng==4){"
                "v += (qrot(r[g.w], pos) + d[g.w]) * w;"
            "}"
        "}"
    "}"
    "gl_Position = matrix * vec4(v, 1.0);"
"}",

0,

"#version 330\n"
"uniform sampler2D zero;"
"uniform vec4 k;"
"uniform vec4 samp;"
"uniform vec3 o;"
"in vec2 x;"
"out vec4 c;"
"out vec4 e;"
"void main(){"
    "vec4 t=texture2D(zero,x);"
    "c=vec4((1.0-k.a)*t.rgb+k.a*((1.0-t.a)*k.rgb+t.a*t.rgb),(1.0-k.a)*t.a+k.a)*samp;"
    "e=vec4(o,t.a);"
"}"
},
{ /* particle */
"#version 330\n"
"layout(location=0)in uvec4 d;"
"out uvec4 data;"
"void main(){"
    "data=d;"
"}",

"#version 330\n"
"layout(points)in;"
"layout(triangle_strip,max_vertices=4)out;"
"uniform mat4 matrix;"
"uniform float k;"
"uniform vec4 info[256];"
"in uvec4 data[];"
"out vec2 t;"
"flat out uint i;"
"void main(){"
    "vec4 p=matrix*vec4(float(data[0].x)/65536.0,float(data[0].y)/65536.0,float(data[0].z)/65536.0,1.0);"
    "vec2 s=vec2(8.0/k,8.0);"
    "uint id=(data[0].w&0xFFu);"
    "vec4 a=info[id];"
    "float x=floor(a.x+a.y*(a.z-float(data[0].w>>16u))/a.z+0.5)/64.0;"

    "i=id;"

    "t=vec2(x,0.0);"
    "gl_Position=p+vec4(-s.x,-s.y,0.0,0.0);"
    "EmitVertex();"

    "t=vec2(x+1.0/64.0,0.0);"
    "gl_Position=p+vec4(s.x,-s.y,0.0,0.0);"
    "EmitVertex();"

    "t=vec2(x,1.0);"
    "gl_Position=p+vec4(-s.x,s.y,0.0,0.0);"
    "EmitVertex();"

    "t=vec2(x+1.0/64.0,1.0);"
    "gl_Position=p+vec4(s.x,s.y,0.0,0.0);"
    "EmitVertex();"

    "EndPrimitive();"
"}",

"#version 330\n"
"uniform sampler2D samp;"
"uniform vec4 color[256];"
"in vec2 t;"
"flat in uint i;"
"out vec4 c;"
"void main(){"
    "c=color[i]*texture2D(samp,t);"
"}"
},
{ /* water */
"#version 330\n"
"layout(location=0)in uvec2 d;"
"out uvec2 data;"
"void main(){"
    "data=d;"
"}",

"#version 330\n"
"layout(points)in;"
"layout(triangle_strip,max_vertices=4)out;"
"uniform mat4 matrix;"
"in uvec2 data[];"
"out vec2 t;"
"flat out vec2 s;"
"void main(){"
    "float x=float((data[0].x&0xFFFu)<<4u);"
    "float y=float((data[0].x>>8u)&0xFFF0u);"
    "float u=float((data[0].x>>24u)&0x3u)/4.0;"
    "float v=float((data[0].x>>26u)&0x3u)/4.0;"
    "float x2=float((data[0].y&0xFFFu)<<4u);"
    "float y2=float((data[0].y>>8u)&0xFFF0u);"
    "float h=float(data[0].y>>24u);"

    "s=vec2(u,v);"

    "t=vec2(x,y);"
    "gl_Position=matrix*vec4(x,y,h,1.0);"
    "EmitVertex();"

    "t=vec2(x2,y);"
    "gl_Position=matrix*vec4(x2,y,h,1.0);"
    "EmitVertex();"

    "t=vec2(x,y2);"
    "gl_Position=matrix*vec4(x,y2,h,1.0);"
    "EmitVertex();"

    "t=vec2(x2,y2);"
    "gl_Position=matrix*vec4(x2,y2,h,1.0);"
    "EmitVertex();"

    "EndPrimitive();"
"}",

"#version 330\n"
"uniform sampler2D samp;"
"in vec2 t;"
"flat in vec2 s;"
"out vec4 c;"
"void main(){"
    "c=texture2D(samp,s+fract(t/64.0)/4.0)*vec4(1.0,1.0,1.0,0.5);"
"}"
},
{ /* strip */
"#version 330\n"
"layout(location=0)in vec3 s;"
"layout(location=1)in vec3 e;"
"layout(location=2)in uvec2 d;"
"out vec3 start;"
"out vec3 end;"
"out uvec2 data;"
"void main(){"
    "start=s;"
    "end=e;"
    "data=d;"
"}",

"#version 330\n"
"layout(points)in;"
"layout(triangle_strip,max_vertices=4)out;"
"uniform mat4 matrix;"
"in vec3 start[];"
"in vec3 end[];"
"in uvec2 data[];"
"out vec2 t;"
"flat out float u;"
"void main(){"
    "vec4 s=matrix*vec4(start[0],1.0);"
    "vec4 e=matrix*vec4(end[0],1.0);"
    "vec2 d=normalize(e.yx-s.yx)*vec2(-1.0,1.0)*4.0;"
    "float x=0.0;"
    "float w=2.0;"
    "float y=0.25;"

    "u=0.25;"

    "t=vec2(x,y);"
    "gl_Position=s+vec4(-d,0.0,0.0);"
    "EmitVertex();"

    "t=vec2(x+w,y);"
    "gl_Position=e+vec4(-d,0.0,0.0);"
    "EmitVertex();"

    "t=vec2(x,y+1.0/16.0);"
    "gl_Position=s+vec4(d,0.0,0.0);"
    "EmitVertex();"

    "t=vec2(x+w,y+1.0/16.0);"
    "gl_Position=e+vec4(d,0.0,0.0);"
    "EmitVertex();"

    "EndPrimitive();"
"}",

"#version 330\n"
"uniform sampler2D samp;"
"in vec2 t;"
"flat in float u;"
"out vec4 c;"
"void main(){"
    "c=texture2D(samp,vec2(u+fract(t.x)/4.0,t.y));"
"}"
},

{ /* gs */
"#version 330\n"
"layout(location=0)in vec3 p;"
"layout(location=1)in uint d;"
"out vec3 pos;"
"out uint data;"
"void main(){"
    "pos=p;"
    "data=d;"
"}",

"#version 330\n"
"layout(points)in;"
"layout(triangle_strip,max_vertices=4)out;"
"uniform mat4 matrix;"
"in vec3 pos[];"
"in uint data[];"
"out vec2 t;"
"void main(){"
    "vec4 p=vec4(pos[0],1.0);"
    "float s=float((data[0]>>8u)&0xFFu);"
    "float x=float((data[0])&0x7u)/8.0;"
    "float y=float((data[0]>>3u)&0x7u)/8.0;"

    "t=vec2(x,y);"
    "gl_Position=matrix*(p+vec4(-s,-s,0.0,0.0));"
    "EmitVertex();"

    "t=vec2(x+1.0/8.0,y);"
    "gl_Position=matrix*(p+vec4(s,-s,0.0,0.0));"
    "EmitVertex();"

    "t=vec2(x,y+1.0/8.0);"
    "gl_Position=matrix*(p+vec4(-s,s,0.0,0.0));"
    "EmitVertex();"

    "t=vec2(x+1.0/8.0,y+1.0/8.0);"
    "gl_Position=matrix*(p+vec4(s,s,0.0,0.0));"
    "EmitVertex();"

    "EndPrimitive();"
"}",

"#version 330\n"
"uniform sampler2D samp;"
"in vec2 t;"
"out vec4 c;"
"void main(){"
    "c=texture2D(samp,t);"
"}"
},
};
