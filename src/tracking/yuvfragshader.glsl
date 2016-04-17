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

	float minimum = min(r, min(g, b));
        float maximum = max(r, max(g, b));
	float delta = maximum - minimum;

	float h,s,intensity;

	intensity = maximum;
	
	if (maximum != 0.0) {
		s = delta / maximum;
	}
	else {
		s = 0.0;
		h = 0.0;	
	}

	if (r == maximum) {
		h = (g - b) / delta;
	} else if (g == maximum) {
		h = 2.0 + (b - r) / delta;
        } else {
		h = 4.0 + (r - g) / delta;
	}

	h = h * 60.0;
	if (h < 0.0) {
		h = h + 360.0;
	}

	vec4 res;
	res.r = h / 360.0;
	res.g = s;
	res.b = intensity;
	res.a = 1.0;	

    gl_FragColor = clamp(res,vec4(0),vec4(1));
}
