#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  
#include <float.h>
#include <time.h>
#include <string.h>

#define USERS 943
#define MOVIES 1683
#define MAX_VERTICES (USERS + MOVIES)

// Kullanıcı ve film ilişkisi için yapı
typedef struct Node {
    int movieId;
    int rating;
    struct Node *next;
} Node;

// Kenarı temsil eder
typedef struct Edge {
    int dest;
    float weight;
    struct Edge *next;
} Edge;

// Grafik yapısı
typedef struct Graph {
    int vertices;  //düğüm
    Edge **adjList; //Her düğümün bağlantılarını (komşularını) bir liste şeklinde saklar.
} Graph;

// Global değişkenler
Node *userMovies[USERS + 1] = {NULL}; //userMovies[1]: Kullanıcı 1'in izlediği filmleri gösteren linked listinn başlangıcını iişaret ediyor
int movieDegree[MOVIES + 1] = {0};  // her filmin derecesi (kaç kişi izlemiş)

// Fonksiyon Bildirimleri
void addMovieToUser(int user, int movie, int rating);
void createRecommendationsFile(const char *inputFile, const char *outputFile);
void buildGraphFromFile(const char *fileName, Graph *graph, bool *watchedMovies, int userId);
Graph *createGraph(int vertices);
void addEdge(Graph *graph, int src, int dest, float weight);
void dijkstra(Graph *graph, int src, float *distances);
void recommendNearestMovies(Graph *graph, int userId, bool *watchedMovies, int rec);
void recommendRandom(int user, int rec);
void recommendHighDegree(int user, int rec);
int findMostSimilarUser(int targetUser);
void recommendFromSimilarUser(int targetUser, int similarUser, int rec);
void recommendTopNFromNearest(float *movieDistances, int *movieIds, int count, int n);
void freeGraph();

// Kullanıcıya bir film eklemek için
void addMovieToUser(int user, int movie, int rating) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->movieId = movie;
    newNode->rating = rating;
    newNode->next = userMovies[user];
    userMovies[user] = newNode;
    movieDegree[movie]++;
}//Film userMovies arrayine eklenir ve movieDegree artırılır.


// Yeni bir grafik oluşturma
Graph *createGraph(int vertices) {
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    graph->vertices = vertices;
    graph->adjList = (Edge **)malloc(vertices * sizeof(Edge *));
    for (int i = 0; i < vertices; i++) {
        graph->adjList[i] = NULL;
    }
    return graph;
}



//adjList bir arraydir ve 
//bu arrayin her bir elemanı bir linked list'in başlangıcını işaret eder
// Kenar ekle
void addEdge(Graph *graph, int start, int dest, float weight) {
    Edge *edge = (Edge *)malloc(sizeof(Edge));
    edge->dest = dest;
    edge->weight = weight;
    edge->next = graph->adjList[start];
    graph->adjList[start] = edge;
}

// u.data dosyasını okuyarak interactions.txt oluşturuyor
void createRecommendationsFile(const char *inputFile, const char *outputFile) {
    FILE *inFile = fopen(inputFile, "r"); //okuma modu
    FILE *outFile = fopen(outputFile, "w"); //yama mosu

    if (!inFile || !outFile) {
        printf("Dosyalari acarken hata olustu.\n");
        exit(1);
    }

    int userId, itemId, rating, timestamp;
    while (fscanf(inFile, "%d %d %d %d", &userId, &itemId, &rating, &timestamp) != EOF) {
        fprintf(outFile, "%d %d %d\n", userId, itemId, rating);
    }

    fclose(inFile);
    fclose(outFile);
}

// Grafiği dosyadan oluştur
void buildGraphFromFile(const char *fileName, Graph *graph, bool *watchedMovies, int userId) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        printf("Dosya acilamadi: %s\n", fileName);
        exit(1);
    }

    int userIdFromFile, itemId, rating;
    while (fscanf(file, "%d %d %d", &userIdFromFile, &itemId, &rating) != EOF) {
        addMovieToUser(userIdFromFile, itemId, rating);

        int userNode = userIdFromFile - 1;
        int movieNode = USERS + (itemId - 1);
        float weight = 1.0 / rating; // Ağırlık hesaplama 1/rating yaptım çünkü 4. öneride en yakına gitmesini istiyorum yani en yüksek puanlıya
        addEdge(graph, userNode, movieNode, weight);
        addEdge(graph, movieNode, userNode, weight);

        if (userIdFromFile == userId) {
            watchedMovies[itemId - 1] = true;
        }
    }

    fclose(file);
}

// Dijkstra algoritması
void dijkstra(Graph *graph, int src, float *distances) {
    int V = graph->vertices;
    bool *visited = (bool *)calloc(V, sizeof(bool));  //düğümün ziyaret edilip edilmediğini tutuyor
    int visitedCount = 0; 

    for (int i = 0; i < V; i++) {
        distances[i] = FLT_MAX;  //en başta hepsi kaynak düğüme sonsuz uzaklıkta
    }
    distances[src] = 0.0; //kaynak düğüm 

    for (int count = 0; count < V; count++) {
        float minDist = FLT_MAX; 
        int minIndex = -1;
        for (int v = 0; v < V; v++) {
            if (!visited[v] && distances[v] < minDist) {
                minDist = distances[v];
                minIndex = v;
            }
        }

        if (minIndex == -1) {
            break;
        }

        visited[minIndex] = true;  // kaynağa en yakın olan düğüm bulunur.
        visitedCount++;

        Edge *edge = graph->adjList[minIndex]; 
        while (edge != NULL) {  //Seçilen düğümün komşuları dolaşılır.
            int dest = edge->dest;
            float weight = edge->weight;
            if (!visited[dest] && distances[minIndex] + weight < distances[dest]) {
                distances[dest] = distances[minIndex] + weight;
            }  //kaynak düğümden diğer düğümlere olan mesafeler distances içinde depolanır
            edge = edge->next;
        }
    } //kaynak düğümden (bizim öneri istediğimiz kullanıcı) ulaşılabilecek her yere ulaşıp onların mesafesini bulana kadar devam eder
    printf("Ziyaret edilen düğüm sayısı: %d\n", visitedCount);

    free(visited);
}

// En yakın filmleri bul ve belirli bir sayı öner
void recommendNearestMovies(Graph *graph, int userId, bool *watchedMovies, int rec) {  //Kullanıcının izlediği filmleri tutan dizi.
    float *distances = (float *)malloc(graph->vertices * sizeof(float));
    int userNode = userId - 1;

    dijkstra(graph, userNode, distances);  //Dijkstra algoritması çağrılarak kullanıcıdan tüm düğümlere olan mesafeler hesaplanır.


    float movieDistances[MOVIES];
    int movieIds[MOVIES];
    int movieCount = 0;

    for (int i = USERS; i < USERS + MOVIES; i++) { //Kullanıcının izlemediği filmler movieDistances ve movieIds arraylerine eklenir.
        int movieId = i - USERS + 1;
        if (!watchedMovies[movieId - 1] && distances[i] < FLT_MAX) {
            movieDistances[movieCount] = distances[i];
            movieIds[movieCount] = movieId;
            movieCount++;
        }
    }

    // Tüm filmler arasında mesafesi en kısa olanlar belirlenir.
    float minDistance = FLT_MAX;
    for (int i = 0; i < movieCount; i++) {
        if (movieDistances[i] < minDistance) {
            minDistance = movieDistances[i];
        }
    }

    int closestCount = 0;
    int closestMovies[MOVIES];   // en yakın mesafedekileri bir dizide saklarız
    for (int i = 0; i < movieCount; i++) {
        if (movieDistances[i] == minDistance) {
            closestMovies[closestCount++] = movieIds[i];
        }
    }

    // Bu en yakın mesafedeki filmlerden rastgele öneriler yapar, bunu yapmadığımda hep en küçük numaralı filmleri önerdi ona çözüm olsun diye böyle yaptım.
    if (closestCount > 0) {
        srand(time(NULL));
        for (int i = 0; i < rec && i < closestCount; i++) {
            int randomIndex = rand() % closestCount;
            printf("Film %d \n", closestMovies[randomIndex]);

            // Seçilen filmi listeden çıkar
            for (int j = randomIndex; j < closestCount - 1; j++) {
                closestMovies[j] = closestMovies[j + 1];
            }
            closestCount--;
        }
    } else {
        printf("Hicbir film bulunamadi.\n");
    }

    free(distances);
}

// Rastgele öneri (soru 1)
void recommendRandom(int user, int rec) {
    int watched[MOVIES + 1] = {0};
    int totalMovies[MOVIES]; //izlenmemiş filmlerin idsini tutuyor
    int movieCount = 0;

    Node *current = userMovies[user]; // kullanıcının izlediği filmlerin başını gösteriyoruz
    while (current != NULL) {
        watched[current->movieId] = 1; //izlenen filmler 1 olarak işaretlenir
        current = current->next;
    }

    for (int i = 1; i <= MOVIES; i++) {
        if (!watched[i]) { // izlenmeyen filmler 0 ya onları buluyoruz
            totalMovies[movieCount++] = i; // izlenmeyen filmleri buraya ekliyoruz
        }
    }

    if (movieCount > 0) {
        srand(time(NULL));
        printf("\nSoru 1: Rastgele Film Onerileri: ");
        for (int i = 0; i < rec && i < movieCount; i++) {
            int randomIndex = rand() % movieCount;
            printf("%d ", totalMovies[randomIndex]);
        }
        printf("\n");
    } else {
        printf("Yeterli izlenmeyen film yok.\n");
    }
}

// Derecesi en yüksek öneri (soru 2) Derecesi en yüksek yani en çok izlenen
void recommendHighDegree(int user, int rec) {
    int watched[MOVIES + 1] = {0};  // Tüm filmleri izlenip izlenmediğine göre takip eden array
    int recommendedMovies[rec];  // Kullanıcıya önerilecek filmler
    int maxDegrees[rec];  // Derecesi en yüksek olan filmleri tutacak array

    // Başlangıçta, öneri listesinde -1 var, bir öneri yapılmadığı için

    for (int i = 0; i < rec; i++) {
        recommendedMovies[i] = -1;
        maxDegrees[i] = -1;
    }

    Node *current = userMovies[user];
    while (current != NULL) {
        watched[current->movieId] = 1;  //kullanıcının izlediği filmler watched dizisine eklenir 1 olarak
        current = current->next;
    }

    for (int movie = 1; movie <= MOVIES; movie++) {
        if (!watched[movie]) {    //film izlenmemişse
            int degree = movieDegree[movie]; //derecesini al

// Dereceye göre en yüksek olan filmleri bulmak için bir iç döngü 
            for (int i = 0; i < rec; i++) {
                if (degree > maxDegrees[i]) {  // Eğer şu anki filmin derecesi daha yüksekse Önceki filmler sağa 
                    for (int j = rec - 1; j > i; j--) {
                        maxDegrees[j] = maxDegrees[j - 1];  // Dereceleri kaydır
                        recommendedMovies[j] = recommendedMovies[j - 1];  // Filmleri kaydır
                    }
                    maxDegrees[i] = degree;  // Yeni filmin derecesi eklenir
                    recommendedMovies[i] = movie;  // Film öneri listesine eklenir
                    break;  // Yüksek dereceye sahip film bulunduğunda döngüden çıkılır
                }
            }
        }  //insertion sort ama büyükten küçüğe sıralanmışı 
    }


    printf("Soru 2: Derecesi En Yuksek Filmlerden Oneriler: ");
    for (int i = 0; i < rec; i++) {
        if (recommendedMovies[i] != -1) {
            printf("%d ", recommendedMovies[i]);
        }
    }
    printf("\n");
}

// Benzer kullanıcıyı bul
int findMostSimilarUser(int targetUser) {
    int commonMovies[USERS + 1] = {0}; //tüm kullanıcılar istenen kullanıcıya ne kadar benzer bulmak için

    Node *targetMovies = userMovies[targetUser];
    while (targetMovies != NULL) {
        for (int user = 1; user <= USERS; user++) {
            if (user == targetUser) continue;

            Node *current = userMovies[user];
            while (current != NULL) {
               if (current->movieId == targetMovies->movieId) {  // Eğer "targetUser" ile ortak bir film bulunursa:
                    commonMovies[user]++;  // Bu kullanıcının ortak film sayısı artırılır.
                    break;
                }
                current = current->next;
            }
        }
        targetMovies = targetMovies->next;
    }

    int maxCommon = -1;
    int mostSimilarUser = -1;
    for (int user = 1; user <= USERS; user++) {
        if (commonMovies[user] > maxCommon) {
            maxCommon = commonMovies[user];     //en çok benzeyen kullanıcı olmak için kullanıcılar karşılaştırılır
            mostSimilarUser = user;
        }
    }

    return mostSimilarUser;
}

// Benzer kullanıcı önerileri
void recommendFromSimilarUser(int targetUser, int similarUser, int rec) {
// Benzer kullanıcının 5 puan verdiği ve "targetUser" tarafından izlenmeyen filmleri öneren fonksiyon başlıyor.

    int watched[MOVIES + 1] = {0};

    Node *current = userMovies[targetUser];
    while (current != NULL) {
        watched[current->movieId] = 1;
        current = current->next;
    }

    Node *similarMovies = userMovies[similarUser];
    printf("Benzer Kullanicinin 5 Puan Verdigi Oneriler: ");
    int found = 0;
    int recommendationsCount = 0;

    while (similarMovies != NULL) {
        if (similarMovies->rating == 5 && !watched[similarMovies->movieId]) {
    // Eğer bulduğumuz benzer kullanıcı 5 puan vermişse ve bu film kullanıcımız (targetu)tarafından izlenmemişse doğru olur

            printf("%d ", similarMovies->movieId);
            found = 1;
            recommendationsCount++;
        }

        if (recommendationsCount == rec) {
            break;
        }

        similarMovies = similarMovies->next;
    }

    if (!found) {
        printf("Hicbir oneri bulunamadi.\n");
    }
    printf("\n");
}

// Hafızayı temizle
void freeGraph() {
    for (int i = 1; i <= USERS; i++) {
        Node *current = userMovies[i];
        while (current != NULL) {
            Node *temp = current;
            current = current->next;
            free(temp);
        }
        userMovies[i] = NULL;
    }
}

// Ana Fonksiyon
int main() {
    const char *inputFile = "u.data";
    const char *outputFile = "interactions.txt";

    Graph *graph = createGraph(MAX_VERTICES);
    bool watchedMovies[MOVIES] = {false};

    createRecommendationsFile(inputFile, outputFile);

    int user;
    int rec;
    printf("Film onerisi almak istediginiz kullanici ID'sini girin (1-%d): ", USERS);
    scanf("%d", &user);

    printf("Kac tane oneri almak istiyorsaniz girin: ");
    scanf("%d", &rec);

    if (user < 1 || user > USERS) {
        printf("Gecersiz kullanici ID'si.\n");
        return 1;
    }

    buildGraphFromFile(outputFile, graph, watchedMovies, user); //void addEdge fonksiyonunu da içinde kullanarak graphımızı oluşturur

    // Rastgele Oneriler
    recommendRandom(user, rec);
    printf("\n");



    // Derecesi En Yuksek Filmler
    recommendHighDegree(user, rec);


    
    int similarUser = findMostSimilarUser(user);
    if (similarUser != -1) {
        printf("\nSoru 3: Benzer Kullanici (%d) Onerileri:\n", similarUser);
        recommendFromSimilarUser(user, similarUser, rec);

    }

    printf("\nSoru 4: En Yakin Mesafedeki Filmlerden Oneriler:\n");
    recommendNearestMovies(graph, user, watchedMovies, rec);


    freeGraph();
    return 0;
}


// array- linked list kullandım çünkü O(1) sürede filmlere ulaşabilirim, linked list olsaydı O(n) kadar sürerdi
// ayrıca boyutu biliyorum o yüzden önceden hafızada o boyutta array kadar yer ayırdım. (film sayısı kadar 943)
// linked list-linked list düğüm sayılarının daha önceden bilinmediği esnek yapılarda kullanılır biz düğüm sayısını zaten biliyoruz kodu karmaşıklaştırmaya gerek yok
