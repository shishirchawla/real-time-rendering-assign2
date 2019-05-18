// Fragment shader

uniform int lighting_model; // Bling-Phong/Phong
uniform int shader_type; // Vertex/Fragment
uniform int normal_view; // Normal(N) View

// Varying variables from vertex shader
varying vec4 ambient, ambientGlobal;
varying vec3 normal, ecPos, lightDir, halfVector;

void main(void)
{
  vec3 n, l, halfV;
  float NdotL, NdotHV;
  vec4 color = vec4(0.0);

  if (shader_type == 1 && normal_view == 0)
  {
    // calculate attenuation again with interpolated light direction (varying length)
    float lightAtt;
    if (gl_LightSource[0].position.w != 0.0) // Positional light
    {
      float dist = length(lightDir);
      lightAtt = min((1.0 / (gl_LightSource[0].constantAttenuation +
              gl_LightSource[0].linearAttenuation * dist +
              gl_LightSource[0].quadraticAttenuation * dist * dist)), 1.0);
    }
    else // Directional light
    {
      lightAtt = 1.0;
    }

    // re-normalize normal and light
    n = normalize(normal);
    l = normalize(lightDir);
    NdotL = max(dot(n, l),0.0);

    // Add ambient component
    color += ambientGlobal + (lightAtt * ambient);

    // Add diffuse component
    color += lightAtt * (NdotL * gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse);

    // Add specular component
    if (NdotL > 0.0)
    {
      if (lighting_model == 0)
      {
        halfV = normalize(halfVector);
        NdotHV = max(dot(n,halfV),0.0);
        color += lightAtt * (gl_FrontMaterial.specular * gl_LightSource[0].specular * 
            pow(NdotHV,gl_FrontMaterial.shininess));
      } else {
        vec3 eye = normalize(-ecPos);	
        vec3 reflection = normalize(-reflect(l,n));
        float RdotEye = max(dot(reflection,eye),0.0);
        color += lightAtt * gl_FrontMaterial.specular * gl_LightSource[0].specular * 
          pow(RdotEye, gl_FrontMaterial.shininess);
      }
    }

    gl_FragColor = color;
  } else { // pass through
    gl_FragColor = gl_Color;
  }
}
