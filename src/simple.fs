in vec3 fsPos;

out vec3 color;

void main() {
    color = fsPos;
    vec3 tex =  vec3(1.0, 0,0 );

    color = tex;
}
