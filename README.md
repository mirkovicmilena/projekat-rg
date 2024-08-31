# project_base
Prazan projekat sa pratećim bibliotekama koje koristimo na času. 

# Uputstvo
1. `https://github.com/mirkovicmilena/projekat-rg.git`
2. CLion -> Open -> path/to/my/project_base
4. ALT+SHIFT+F10 -> project_base -> run

# Keyboard instractions:
Moving:
'W' - forwards
'S' - backwards
'A' - left
'D' - right

# Implemented techniques:
- Required:
  - Blanding
  - Face culling
  - Advanced lighting
- Models
- Group A:
  - Cubemaps - skybox

- Group B:
  - Parallax mapping (incomplete, not working :))

# Technical Explanation
The vertex coordinates of the river were calculated in a for loop ranging from -15 and 15, with step 1.2, and -(sin(i/2) + 4), as well as -(sin(i/2) + i/10, +1). 
The function were chosen after testing in an online graph plotting tool and then calculated in a independent C++ program, witch output the complete 'riverVertex' and 'riverIndices'.

# Computer Graphics
Year: 2023/2024
Teaching Assistant: Marko Spasic
Professor: Vesna Marinkovic
