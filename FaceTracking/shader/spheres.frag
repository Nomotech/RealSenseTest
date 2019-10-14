#version 430 core

uniform vec2 resolution;
uniform int time;
uniform vec3 eye;
uniform int status;

out vec4 FragColor;

const float pi = 3.1415926535;
const float pixelW = 0.180;
const float subPixelW = pixelW / 3.0;

vec3 HSVtoRGB(vec3 hsv) {
    vec3 col;
    float hue = mod(hsv.r, 360.0);
    float s = max(0.0, min(1.0, hsv.g));
    float v = max(0.0, min(1.0, hsv.b));
    if (s > 0.0) {
        int h = int(floor(hue / 60.0));
        float f = hue / 60.0 - float(h);
        float p = v * (1.0 - s);
        float q = v * (1.0 - f * s);
        float r = v * (1.0 - (1.0 - f) * s);

        if (h == 0)
            col = vec3(v, r, p);
        else if (h == 1)
            col = vec3(q, v, p);
        else if (h == 2)
            col = vec3(p, v, r);
        else if (h == 3)
            col = vec3(p, q, v);
        else if (h == 4)
            col = vec3(r, p, v);
        else
            col = vec3(v, p, q);
    } else {
        col = vec3(v);
    }
    return col;
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Light {
    vec3 color;
    vec3 direction;
};

struct Material {
    vec3 color;
    float diffuse;
    float specular;
};

struct Intersect {
    float len;
    vec3 normal;
    Material material;
};

struct Sphere {
    float radius;
    vec3 position;
    Material material;
};

struct Box {
    vec3 normal;
    Material material;
};

const float epsilon = 1.0;
const int iterations = 16;

const float exposure = 0.01;
const float gamma = 2.2;
const float intensity = 100.0;
const vec3 ambient = vec3(0.01, 0.01, 0.01) * intensity / gamma;

Light light = Light(vec3(1.0) * intensity, normalize(vec3(0.0, 1.0, 1.0)));

const Intersect miss = Intersect(0.0, vec3(0.0), Material(vec3(0.0), 0.0, 0.0));

Intersect intersect(Ray ray, Sphere sphere) {
    // Check for a Negative Square Root
    vec3 oc = sphere.position - ray.origin;
    float l = dot(ray.direction, oc);
    float det = pow(l, 2.0) - dot(oc, oc) + pow(sphere.radius, 2.0);
    if (det < 0.0) return miss;

    // Find the Closer of Two Solutions
    float len = l - sqrt(det);
    if (len < 0.0) len = l + sqrt(det);
    if (len < 0.0) return miss;
    return Intersect(len, (ray.origin + len * ray.direction - sphere.position) / sphere.radius, sphere.material);
}

// 空間上の 3角玉との当たり判定をする
Intersect trace(Ray ray) {
float time = 0.0;
    const int num_spheres = 3;
    Sphere spheres[num_spheres];
    spheres[0] = Sphere(
        10.0,
        vec3(
            0.0, 
            0.0, 
            30.0 * cos(time / 150.0 + pi)),
        Material(vec3(0.6, 1.0, 0.6), 0.5, 0.25)
    );
    spheres[1] = Sphere(
        20.0,
        vec3(
            50.0, 
            30.0 * cos(time / 200.0), 
            -10.0),
        Material(vec3(1.0, 0.6, 0.6), 1.0, 0.1)
    );
    spheres[2] = Sphere(
        20.0,
        vec3(
            -50.0 + 30.0 * cos(time / 100.0),
            20.0 * sin(time / 100.0),
            30.0 * cos(time / 100.0)),
        Material(vec3(0.6, 0.6, 1.0), 1.0, 0.001)
    );

    Intersect intersection = miss;
    for (int i = 0; i < num_spheres; i++) {
        Intersect sphere = intersect(ray, spheres[i]);
        if (sphere.material.diffuse > 0.0 || sphere.material.specular > 0.0)
            intersection = sphere;
    }
    return intersection;
}

vec3 radiance(Ray ray) {
    vec3 color = vec3(0.0), fresnel = vec3(0.0);
    vec3 mask = vec3(1.0);
    for (int i = 0; i <= iterations; ++i) {
        Intersect hit = trace(ray);

        if (hit.material.diffuse > 0.0 || hit.material.specular > 0.0) {
            vec3 r0 = hit.material.color.rgb * hit.material.specular;
            float hv = clamp(dot(hit.normal, -ray.direction), 0.0, 1.0);
            fresnel = r0 + (1.0 - r0) * pow(1.0 - hv, 5.0);
            mask *= fresnel;

            if (trace(Ray(ray.origin + hit.len * ray.direction + epsilon * light.direction, light.direction)) == miss) {
                color += clamp(dot(hit.normal, light.direction), 0.0, 1.0) * light.color * hit.material.color.rgb * hit.material.diffuse * (1.0 - fresnel) * mask / fresnel;
            }

            vec3 reflection = reflect(ray.direction, hit.normal);
            ray = Ray(ray.origin + hit.len * ray.direction + epsilon * reflection, reflection);

        } else {
            vec3 spotlight = vec3(1.0) * 100.0 * pow(abs(dot(ray.direction, light.direction)), 360.0);
            color += mask * (ambient + spotlight);
            break;
        }
    }
    return color;
}

void main(void) {
    int step = int(time / 2) % 4 * 3;   // barrier は 4分割 幅 3 sub pixel
    vec3 color = vec3(0.0);

    vec3 p = vec3((gl_FragCoord.xy - resolution / 2.0) * pixelW, 0.0);
    Ray ray = Ray(eye, normalize(p - eye));    
    color = pow(radiance(ray) * exposure, vec3(1.0 / gamma));

    // status
    float frameWidth = 20.0;
    if (   gl_FragCoord.x < frameWidth 
        || gl_FragCoord.x > resolution.x - frameWidth 
        || gl_FragCoord.y < frameWidth 
        || gl_FragCoord.y > resolution.y - frameWidth) {
            if (status == 0) { // tracking off
                color = vec3(1.0, 0.2, 0.0);
            } else if (status == 1) { // tracking on
                color = vec3(0.0, 1.0, 0.0);
            }
    }

    FragColor = vec4(color, 1.0);
}