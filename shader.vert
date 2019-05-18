// shader.vert

// Ambient and diffuse lighting shader

uniform int lighting_model; // Bling-Phong(0), Phong(1)
uniform int shader_type; // Vertex (0), Fragment(1)
uniform int shape; // Shape Type - sphere(0), torus(1), grid(2)
uniform int bumps; // Bump Type - no-bumps(0), only-normals(1), with-displacement(2)
uniform int viewer; // Viewer - infinite(0) or local(1)
uniform int normal_view; // normal-visual-disabled(0), normal-visual-enabled(1)

// pass normal, eye position and related variables to fragment shader for interpolation
varying vec4 ambient, ambientGlobal;
varying vec3 normal, ecPos, lightDir, halfVector;

void main(void)
{
  // Vertex color
  vec4 color = vec4(0.0);

  // Calculate vertex and normal coordinates from parametrics u and v
  float pi = acos(-1.0);
  float u, v;
  vec3 N, V, T, B;

  // Also calculate tangent and binormal vectors for bump mapping and displacement
  if (shape == 0) // Sphere
  {
    u = gl_Vertex.x * (2.0 * pi);
    v = gl_Vertex.y * pi;

    float radius = 1.0;

    N.x = cos(u) * sin(v);
    N.y = sin(u) * sin(v);
    N.z = cos(v);
    V.x = radius * N.x;
    V.y = radius * N.y;
    V.z = radius * N.z;

    T.x = -sin(u) * sin(v);
    T.y = cos(u) * sin(v);
    T.z = cos(v);
  }
  else if (shape == 1) // Torus
  {
    u = gl_Vertex.y * 2.0 * pi;
    v = gl_Vertex.x * 2.0 * pi;

    float R = 1.0;
    float r = 0.5;

    N.x = cos(u) * cos(v);
    N.y = sin(u) * cos(v);
    N.z = sin(v);
    V.x = (R + r * cos(v)) * cos(u);
    V.y = (R + r * cos(v)) * sin(u);
    V.z = r * sin(v);

    T.x = cos(u) * -sin(v);
    T.y = sin(u) * -sin(v);
    T.z = cos(v);
  }
  else if (shape == 2) // Grid
  {
    u = gl_Vertex.x;	
    v = gl_Vertex.y;	

    V.x = (u - 0.5)*2.0;
    V.y = (v - 0.5)*2.0;
    V.z = 0.0;
    N.x = 0.0;
    N.y = 0.0;
    N.z = 1.0;

    T.x = 1.0;
    T.y = 0.0;
    T.z = 0.0;
  }

  B.x =   (N.y * T.z) - (N.z * T.y);
  B.y = -((N.x * T.z) - (N.z * T.x));
  B.z =   (N.x * T.y) - (N.y * T.x);

  // if bump state enabled - calculate bump normals
  if (bumps > 0) {
    // bump displacement
    float bumpDensity = 16.0;
    float bumpSize = 0.25;
    vec2 c = bumpDensity * gl_Vertex.xy;
    vec2 p = fract(c) - vec2(0.5);
    float d, f;
    d = (p.x * p.x) + (p.y * p.y);
    f = 1.0 / sqrt(d + 1.0);
    if (d >= bumpSize) { 
      p = vec2(0.0);
      f = 1.0;
    }

    // if bump displacement enabled - calculate and apply displacement to vertex
    if (bumps == 2) {
      float vdis = sqrt(0.25 - sqrt(d) * sqrt(d)) * 0.16;
      if (vdis > 0.0)
        V += N * vdis;
    }

    // bump Normal
    float dis = sqrt(0.25 - sqrt(d) * sqrt(d));
    if (dis > 0.0) {
      normal = vec3(p.x, -p.y, dis) * f;
      // convert to eye space
      normal = T * normal.x + B * normal.y + N * normal.z;
    }
    else // shape normal
      normal = N;

  } else {
    // shape Normal
    normal = N;
  }

  //end

  // Normalized vertex normal
  normal = normalize(vec3(gl_NormalMatrix * normalize(normal)));

  // If normal view enabled - render normal as colors (notice - n+1.0/2.0 to get a color between 0 and 1)
  if (normal_view == 1)
    gl_FrontColor = vec4(((normal.x + 1.0)/2.0),((normal.y + 1.0)/2.0),((normal.z + 1.0)/2.0), 1.0);

  // Vertex position in camera space
  if (viewer == 0) // if viewer at inifinity
    ecPos = vec3(gl_ModelViewMatrix * vec4(0.0, 0.0, V.z, 1.0));
  else // if viewer local
    ecPos = vec3(gl_ModelViewMatrix * vec4(V, 1.0));

  /* Calculate Light direction, Attenuation and Half Vector on basis of type of light. */
  float lightAtt;
  if (gl_LightSource[0].position.w != 0.0) // Positional light
  {
    // opengl fixed pipeline treats viewer as local when light positional
    vec3 ecPosPL = vec3(gl_ModelViewMatrix * vec4(V, 1.0));
    // vector from light to vertex
    lightDir = vec3(gl_LightSource[0].position) - ecPosPL;
    // calculate length for attenuation
    float dist = length(lightDir);
    lightAtt = 1.0 / (gl_LightSource[0].constantAttenuation +
        gl_LightSource[0].linearAttenuation * dist +
        gl_LightSource[0].quadraticAttenuation * dist * dist);

    // tweaks to half vector based on viewer position
    if (viewer == 0) {
      vec3 eye = vec3(0.0, 0.0, 1.0);
      halfVector = normalize(normalize(lightDir) + eye);
    }
    else  
      halfVector = normalize(normalize(lightDir) - normalize(ecPosPL));
  }
  else // Directional light
  {
    // light dir and attenuation for light at infinity
    lightDir = vec3(gl_LightSource[0].position);
    lightAtt = 1.0;
    halfVector = normalize(normalize(lightDir) - normalize(ecPos));
  }

  // Compute ambient components
  ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
  ambientGlobal = gl_FrontMaterial.ambient * gl_LightModel.ambient; 

  if (shader_type == 0) // if vertex shader is enabled - calculate vertex color
  {
    // Add global and light ambient component
    color += ambientGlobal + (lightAtt * ambient);

    // Unit vector in direction of light in eye space
    // (light position already has modelview matrix applied)
    vec3 light = normalize(lightDir);

    // Add diffuse component
    float NdotL = max(dot(normal, light), 0.0);
    color += lightAtt * (NdotL * gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse);

    // Add specular component
    if (NdotL > 0.0)
    {
      if (lighting_model == 0) // blinn-phong calculate color on basis of half vector
      {
        float NdotHV = max(dot(normal,halfVector),0.0);
        color += lightAtt * gl_FrontMaterial.specular * gl_LightSource[0].specular * 
          pow(NdotHV, gl_FrontMaterial.shininess);
      } else { // phong calculate color on basis of reflection vector
        vec3 eye = normalize(-ecPos);	
        vec3 reflection = normalize(-reflect(light,normal));
        float RdotEye = max(dot(reflection,eye),0.0);
        color += lightAtt * gl_FrontMaterial.specular * gl_LightSource[0].specular * 
          pow(RdotEye, gl_FrontMaterial.shininess);
      }
    }

    // Set the color
    if (normal_view == 0)
      gl_FrontColor = color;
  }

  // Apply matrix transforms to vertex position to give clip space
  gl_Position = gl_ModelViewProjectionMatrix * vec4(V, 1.0);
}
