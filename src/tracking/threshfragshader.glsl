varying vec2 tcoord;
uniform sampler2D tex;
uniform vec4 col_lo;
uniform vec4 col_hi;
void main(void) 
{
    vec4 texcol = texture2D(tex,tcoord);
    bool gt = all(greaterThanEqual(texcol,col_lo));
    bool lt = all(lessThanEqual(texcol,col_hi));
    gl_FragColor = vec4(float(gt && lt), 0, 0, 0);
}
