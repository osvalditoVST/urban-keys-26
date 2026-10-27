#pragma once

struct PresetDef
{
    const char* name;
    const char* tags;
    float rolloff;      // caida de armonicos (mayor = mas oscuro)
    float decay;        // decaimiento natural de la cuerda (s)
    float inharm;       // inarmonicidad de cuerda
    float evenGain;     // ganancia de armonicos pares (<1 = hueco/Rhodes)
    float bell;         // campanas inarmonicas
    float noise;        // ruido de martillo
    float detuneCents;  // desafinacion entre cuerdas del coro
    // valores de perilla que carga el preset
    float cutoff, reso, attackMs, releaseMs, rvbMix, dlyMix, chorusMix;
};

inline const PresetDef kPresets[12] =
{
    { "Coculuela Type Piano", "Urbano  \xc2\xb7  Melanc\xc3\xb3lico  \xc2\xb7  Profundo", 1.5f, 2.2f, 0.0002f, 1.0f, 0.00f, 0.12f,  3.0f, 12000, 0.4f, 10, 1200, 35, 28, 15 },
    { "Dark Piano",           "Oscuro  \xc2\xb7  Suave  \xc2\xb7  Profundo",            2.3f, 2.8f, 0.0002f, 0.9f, 0.00f, 0.08f,  4.0f,  3800, 0.3f, 12, 1800, 40, 15,  5 },
    { "Romantic Piano",       "Rom\xc3\xa1ntico  \xc2\xb7  C\xc3\xa1lido  \xc2\xb7  Amplio", 1.8f, 3.2f, 0.0001f, 1.0f, 0.00f, 0.08f,  5.0f,  9000, 0.3f, 15, 2200, 50, 20, 12 },
    { "Puerto Rico Piano",    "Brillante  \xc2\xb7  Latino  \xc2\xb7  Corto",           1.1f, 1.5f, 0.0004f, 1.1f, 0.05f, 0.22f,  3.0f, 14000, 0.4f,  5,  900, 30, 30,  8 },
    { "Street Piano",         "Calle  \xc2\xb7  Crudo  \xc2\xb7  Urbano",               1.4f, 1.0f, 0.0008f, 1.0f, 0.00f, 0.28f,  9.0f,  8000, 0.45f, 3,  600, 22, 18, 10 },
    { "Trap Piano",           "Trap  \xc2\xb7  Seco  \xc2\xb7  Cristalino",             1.0f, 0.9f, 0.0003f, 0.8f, 0.18f, 0.15f,  2.0f,  9500, 0.35f, 3,  700, 25, 30,  0 },
    { "Piano + Bells",        "Campanas  \xc2\xb7  Brillante  \xc2\xb7  Melod\xc3\xad" "a", 1.6f, 2.0f, 0.0002f, 1.0f, 0.50f, 0.10f,  3.0f, 11000, 0.4f,  6, 1500, 38, 28, 10 },
    { "Vintage Piano",        "Vintage  \xc2\xb7  C\xc3\xa1lido  \xc2\xb7  Chorus",     1.9f, 1.8f, 0.0005f, 0.7f, 0.00f, 0.18f, 12.0f,  5500, 0.35f, 8, 1100, 30, 15, 30 },
    { "Sad Piano",            "Triste  \xc2\xb7  Lento  \xc2\xb7  Emotivo",             2.0f, 3.8f, 0.0001f, 1.0f, 0.00f, 0.06f,  4.0f,  4500, 0.3f, 20, 2500, 55, 25,  8 },
    { "Ambient Piano",        "Ambiente  \xc2\xb7  Et\xc3\xa9reo  \xc2\xb7  Largo",            2.4f, 5.5f, 0.0001f, 0.9f, 0.06f, 0.03f,  6.0f,  5500, 0.35f, 80, 4000, 70, 35, 20 },
    { "Dark Keys",            "Oscuro  \xc2\xb7  Hueco  \xc2\xb7  Grave",               1.2f, 1.7f, 0.0003f, 0.55f, 0.00f, 0.14f,  6.0f,  2600, 0.5f,  8, 1400, 35, 22, 10 },
    { "Main Melody",          "Melod\xc3\xad" "a  \xc2\xb7  Claro  \xc2\xb7  Principal",   1.3f, 1.7f, 0.0002f, 1.0f, 0.10f, 0.12f,  4.0f, 11000, 0.4f,  6, 1300, 32, 28, 12 },
};
