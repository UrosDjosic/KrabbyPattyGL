#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;
uniform bool uLightEnabled;

//material properties
uniform vec3 uMaterialDiffuse;
uniform vec3 uMaterialSpecular;
uniform float uMaterialShininess;
uniform bool uUseMaterial;

//patty cooking color
uniform bool uUseCooked;
uniform float uCookedness; //0.0 = raw (pink), 1.0 = cooked (brown)

void main() {
    vec3 objectColor = texture(uTexture, TexCoords).rgb;
    
    //use cookedness-based color for patty
    if (uUseCooked) {
        vec3 rawColor = vec3(0.9, 0.5, 0.6);      //pink/raw meat color
        vec3 cookedColor = vec3(0.4, 0.25, 0.15); //brown/cooked meat color
        objectColor = mix(rawColor, cookedColor, uCookedness);
    }
    //use material color if enabled
    else if (uUseMaterial) {
        objectColor = uMaterialDiffuse;
    }
    
    if (uLightEnabled) {
        //ambient
        float ambientStrength = 0.3;
        vec3 ambient = ambientStrength * uLightColor;
        
        //diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(uLightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * uLightColor;
        
        //specular
        float specularStrength = 0.5;
        vec3 viewDir = normalize(uViewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterialShininess);
        vec3 specular = specularStrength * spec * uLightColor * uMaterialSpecular;
        
        vec3 result = (ambient + diffuse + specular) * objectColor;
        FragColor = vec4(result, 1.0);
    }
    else {
        //when light is off, show basic shading using normals
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(uViewPos - FragPos);
        float brightness = max(dot(norm, viewDir), 0.0) * 0.5 + 0.5;
        
        vec3 result = objectColor * brightness;
        FragColor = vec4(result, 1.0);
    }
}
