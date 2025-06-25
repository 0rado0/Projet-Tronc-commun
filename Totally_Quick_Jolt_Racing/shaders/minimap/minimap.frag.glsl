#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D minimapTexture; // Renommé pour éviter les conflits

// Position du fragment (entre -1 et 1, pour le cercle centré)
in vec2 fragPos;

void main() {
    // Calculer la distance au centre normalisé (0.5, 0.5)
    float dist = length(fragPos);

    
    // Masquage circulaire
    if (dist > 1.0) {
        discard; // Jeter les fragments en dehors du cercle
    }
    
    FragColor = texture(minimapTexture, TexCoord);
}