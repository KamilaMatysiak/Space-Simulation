# 1 Wstęp 

Celem tych zadań jest zapoznanie się z biblioteką assimp służącą do wczytywania modeli 3d w różnych formatach. 

Projekt VS do nich dołączony jest dobrym punktem wyjścia do zaczęcia projektu zaliczeniowego, ponieważ zawiera wszystkie wymagane biblioteki. 

# 2 Ładowanie pliku 
## 2.1 Export z blendera

Internet jest pełen wszelakiej maści modeli 3d (na końcu jest zbiór linków gdzie szukać modeli). Wiele z nich jest dostępnych za darmo, jednak jak to bywa z rzeczami za darmo zazwyczaj nie będą przygotowane pod nasze potrzeby dlatego, trzeba je chociaż w podstawowym zakresie obrobić i wyeksportować do odpowiedniego formatu. W tym celu najlepiej użyć Blendera, który jest darmowy i posiada szeroką grupę pasjonatów udostępniających modele i poradniki za darmo. 

W folderze model znajdują się pliki `arm.blend` oraz `arm.fbx`. Niestety assimp nie obsługuje plików blend z wersji >2.79 (nie polecam używać blendera poniżej 2.80, interfejs jest wyjątkowo nieczytelny), dlatego modele trzeba eksportować do innego formatu, na nasze potrzeby odpowiedni będzie format **fbx**. 
Otwórz plik `arm.blend` w Blenderze (w oknie ładowania pod kołem zębatym jest opcja **load ui** polecam ją wyłączyć, gdy ładujemy nieswoje projekty), zawiera on proste ramie robota. ma ono strukturę hierarchiczną: przykładowo ramie jest dzieckiem kuli, więc jeśli obrócisz kulę, to ramie za nią podąży. Zachowując tą hierarchię będziemy mogli animować ramie za pomocą grafu sceny. Opcja eksportu znajduje się w File->Export->FBX, w **Include** w object types zaznacz tylko opcję **Mesh**. Upewnij się, że w zakładce **Transform** opcja **Apply Scalinng** jest ustawiona na **FBX Units Scale**. zapisz plik w folderze models.
## 2.2 Import przez assimp
Modele importuje się za pomocą klasy `Assimp::Importer` i metody `ReadFile`. Przyjmuje ona ścieżkę do modelu oraz flagi, które określają jakie kroki ma wykonać importer w postprocesingu, może być to obliczenie przestrzeni stycznej czy triangularyzacja. Metoda zwraca wskaźnik na `aiScene`, jeżeli import się powiódł. Jeżeli nie błąd można pobrać za pomocą `importer.GetErrorString()`. Zmienna `aiScene` zawiera dane o scenie, takie jak *mesh*, informację o materiałach czy hierarchię obiektów w postaci drzewa. Korzeniem drzewa jest `scene->mRootNode`. W naszym przypadku `RootNode` będzie miał jednego syna, czyli **base** w pliku `blend`, on z kolei będzie miał syna **ball** itd. każdy węzęł zawiera informację o konkretnym modelu jak liczba meshy i ich indeksy w scenie, materiały itd. Przykładowo wczytanie samej bazy będzie wyglądać następująco:
```c++
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("models/arm.fbx", aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
	// check for errors
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}
	auto node = scene->mRootNode->mChildren[0];
	aiMesh* mesh = scene->mMeshes[node->mMeshes[0]];
	armContext.initFromAssimpMesh(mesh);
```
By przejść po całej scenie potrzebujemy funkcję rekurencyjną `loadRecusive`, która załaduje meshe z danego węzła, doda je do vectora `armContexts`, następnie wywoła się dla wszystkich jego synów. Napisz taką funkcję i wyświetl wszystkie obiekty z vectora z macierzą modelu ustawioną na identyczność. 
Otrzymaliśmy na obiekty, ale są one wyświetlane jeden na drugim, ponieważ nie wczytaliśmy macierzy opisujących ich położenie. Rozwiążemy to w następnym etapie
# 3 Graf Sceny
Każdy węzeł zawiera atrybut `mTransformation`, który jest macierzą transformacji względem ojca. By na jej podstawie uzyskać macierz modelu zbudujemy prosty graf sceny. Użyjemy do tego vektora. W `RenderUtils.h` znajduje się struktura `Node`, która zawiera:
* `vector<RenderContext>`, w którym będziemy umieszczać dane jego meshy, 
* macierz modelu 
* `int parent`, który będzie zawierał indeks modelu lub -1 jeżeli jest korzeniem.

Przerób funkcję `loadRecusive`. Dodaj argument parent, w którym będziemy przesyłać rekurencyjnie indeks ojca. Zmodyfikuj tak, żeby każde wywołanie dodawało element do `vector<Node> arm` i uzupełniał go o meshe, indeks ojca i macierz transformacji. `node->mTransformation` ma typ `aiMatrix4x4`, musisz ją konwertować za pomocą `Core::mat4_cast`.

Do wyświetlenia wykorzystaj poniższy kod
```C++

	for (auto part : arm) {
		glm::mat4 transformation=part.matrix;
		int parent = part.parent;
			while (parent != -1) {
				transformation = arm[parent].matrix * transformation;
				parent = arm[parent].parent;
		}
		for (auto context : part.renderContexts) drawObject(program, context, transformation, glm::vec3(0.6f));
	}
```
# 4 Animacja 

Dodamy ruch ramienia na klawisze **o** i **p**. By ramie się obracało zgodnie z intuicją wystarczy, że dodamy rotację do kuli (*Ball*) a graf sceny załatwi resztę. Po pierwsze potrzebujemy znać pozycję kuli w vectorze. Dodaj do `loadRecusive` sprawdzenie czy nazwa (znajduje się w `node->mName`) to `aiString("Ball")`, jeżeli tak, przypisz do zmiennej globalnej ballIndex indeks węzła w vectorze `arm`. Pozostaje dodać obsługę przycisków w funkcji `keyboard`. By wykonać obrót przemnóż od prawej macierz kuli o macierz obrotu o mały kąt. Możesz też wykorzystać wejście myszki i obroty za pomocą kwaternionów z najnowszych ćwiczeń

# linki
* https://assimp-docs.readthedocs.io/ dokumentacja assimp
* https://learnopengl.com/Model-Loading/Assimp tutorial w learnopengl
* https://www.blender.org/download/ blender

## modele 3d
* https://www.blendswap.com
* https://sketchfab.com
* https://www.cgtrader.com/
* https://www.turbosquid.com/
* https://open3dmodel.com/