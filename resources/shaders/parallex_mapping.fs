#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

struct DirLight{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;

    sampler2D depth;
    float shininess;
};

uniform Material material;
uniform vec3 viewPos;
uniform PointLight pointLight;
uniform DirLight dirLight;

uniform float heightScale;

vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords);
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords);

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{

    // number of depth layers
        const float minLayers = 8;
        const float maxLayers = 32;
        float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
        // calculate the size of each layer
        float layerDepth = 1.0 / numLayers;
        // depth of current layer
        float currentLayerDepth = 0.0;
        // the amount to shift the texture coordinates per layer (from vector P)
        vec2 P = viewDir.xy / viewDir.z * heightScale;
        vec2 deltaTexCoords = P / numLayers;

        // get initial values
        vec2  currentTexCoords     = texCoords;
        float currentDepthMapValue = texture(material.depth, currentTexCoords).r;

        while(currentLayerDepth < currentDepthMapValue)
        {
            // shift texture coordinates along direction of P
            currentTexCoords -= deltaTexCoords;
            // get depthmap value at current texture coordinates
            currentDepthMapValue = texture(material.depth, currentTexCoords).r;
            // get depth of next layer
            currentLayerDepth += layerDepth;
        }

        // get texture coordinates before collision (reverse operations)
        vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

        // get depth after and before collision for linear interpolation
        float afterDepth  = currentDepthMapValue - currentLayerDepth;
        float beforeDepth = texture(material.depth, prevTexCoords).r - currentLayerDepth + layerDepth;

        // interpolation of texture coordinates
        float weight = afterDepth / (afterDepth - beforeDepth);
        vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

        return finalTexCoords;
}

void main()
{
    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoords;

    texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

    // obtain normal from normal map
    vec3 normal1 = texture(material.normal, texCoords).rgb;
    normal1 = normalize(normal1 * 2.0 - 1.0);

    vec4 result = CalcDirLight(dirLight, normal1, viewDir, texCoords);
    result += CalcPointLight(pointLight, normal1, fs_in.FragPos, viewDir, texCoords);

    FragColor = result;
}

vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords){

        vec3 lightDir = normalize(-light.direction);
        float diff = max(dot(normal, lightDir), 0.0);

        vec3 reflection = reflect(-lightDir, normal);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

        vec4 ambient = vec4(light.ambient, 1.0) * texture(material.diffuse, texCoords);
        vec4 diffuse = vec4(light.diffuse * diff, 1.0) * texture(material.diffuse, texCoords);
        vec4 specular = vec4(light.specular * spec, 1.0) * texture(material.specular, texCoords);

        return (ambient + diffuse + specular);
}

vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords){

        vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
        // diffuse shading
        float diff = max(dot(normal, lightDir), 0.0);
        // specular shading
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
        // attenuation
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        // combine results
        vec4 ambient = vec4(light.ambient, 1.0) * texture(material.diffuse, texCoords);
        vec4 diffuse = vec4(light.diffuse * diff, 1.0) * texture(material.diffuse, texCoords);
        vec4 specular = vec4(light.specular * spec, 1.0) * texture(material.specular, texCoords);
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;
        return (ambient + diffuse + specular);
}