varying vec2 tcoord;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
void main(void) 
{
	float y = texture2D(tex0,tcoord).r;
	float u = texture2D(tex1,tcoord).r;
	float v = texture2D(tex2,tcoord).r;

	float r = (y + (1.370705 * (v-0.5)));
	float g = (y - (0.698001 * (v-0.5)) - (0.337633 * (u-0.5)));
	float b = (y + (1.732446 * (u-0.5)));
	vec3 c = vec3(r,g,b);

	vec3 rgb = vec3(r,g,b);
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
	
	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
    	vec3 hsv = vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
	
	vec4 res = vec4(hsv, 0);

	gl_FragColor = clamp(res,vec4(0),vec4(1));
}
