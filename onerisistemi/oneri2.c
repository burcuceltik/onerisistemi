#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>    

#define MAX_USERS 943
#define MAX_MOVIES 1682
#define MAX_GENRES 19

// Kullanıcılar x Filmler için adjacency matrix (Bipartite Graph)
int bipartite_matrix[MAX_USERS][MAX_MOVIES];  // Kullanıcılar ve Filmler arasındaki ilişkiyi gösterir

/**adjency matrix kullandım çünkü Hızlı erişim: Bir kullanıcının bir filmi izleyip izlemediğini
 *  kontrol etmek için O(1) zaman karmaşıklığı. Aşırı büyük boyutlu dosyalar kullanmazsan daha hızlı 
 * 943 × 1682 = 1,586,626 hücre Her hücre için 4 bayt (int) kullanırsak: 6 MB bellek gereklidir.
 * bizim için çok sorun yaratacak bir hafıza kaplamıyor daha hızlı olacağı için bunu kullandım
 * ama şunu unutmamak lazım bir kullanıcı 1682 filmin çok çok büyük bir kısmını izlemediği için baya boş kutucuk kalacak
 * (bir de diğerlerinde hep adjency list kullandım diye farklı olsun istedim açıkası**/

//Eğer bir kullanıcı bir filmi izlediyse o hücrede verdiği rating tutulur, izlemediyse 0 tutulut

// Filmlerin türlerini tutan dizi, toplam 19 tür var bir film birden fazla tür olabilir
int movie_genres[MAX_MOVIES][MAX_GENRES];  // Filmlerin türlerini tutar

// Türlerin isimleri
const char *genres[MAX_GENRES] = {
    "Unknown", "Action", "Adventure", "Animation", "Children's", "Comedy", "Crime", "Documentary", 
    "Drama", "Fantasy", "Film-Noir", "Horror", "Musical", "Mystery", "Romance", "Sci-Fi", "Thriller", 
    "War", "Western"
};

// Kullanıcının hangi filmi ne kadar izlediğini tutar
int user_genre_count[MAX_USERS][MAX_GENRES];

// Kullanıcıların en çok izlediği türü bulma
void find_favorite_genre(int user_id, int *favorite_genre, int *max_count) {
    *max_count = 0;
    *favorite_genre = -1;

    // Türlerin sayısını kontrol et ve en çok izlenen türü bul
    for (int i = 0; i < MAX_GENRES; i++) {
        if (user_genre_count[user_id][i] > *max_count) {  // her tür için tek tek bakar ve en çok izleneni bulur
            *max_count = user_genre_count[user_id][i];
            *favorite_genre = i;
        }  //user_genre_count dizisindeki en büyük değeri arar.
    }
}
//Döngü tamamlandığında: *favorite_genre: Kullanıcının en çok izlediği türün indeksini (örneğin, 1 → "Action") içerir.
//*max_count: Bu türde kaç film izlendiğini içerir.


// Adjacency Matrix Yapısı ile Graph Oluşturma 
void build_bipartite_matrix(const char *filename) {
    FILE *file = fopen(filename, "r"); //dosyayı okumak için açar
    if (file == NULL) {
        printf("Veri dosyasi acilamadi!\n");
        exit(1);
    } //u.data dosyasını okuyarak kullanıcıların izlediği filmleri ve verdikleri ratingleri matrise işler.

    char line[1024];
    while (fgets(line, sizeof(line), file)) {  //file: Açılan dosyayı temsil eder. Dosyadan bir satır okur ve line dizisine yazar.
        int user_id, movie_id, rating;
        sscanf(line, "%d\t%d\t%d", &user_id, &movie_id, &rating);
        bipartite_matrix[user_id - 1][movie_id - 1] = rating;  // Kullanıcı ve filmi ilişkilendiriyoruz
    } //diziler 0dan başladığı için -1

    fclose(file);
}

// Film türlerini okuyarak ilişkileri bipartite_matrix ile eşleştirme
void build_movie_genres(const char *filename) {  //u.item
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Film turleri dosyasi acilamadi!\n");
        exit(1);
    }

    char line[1024]; // 1024 sayısı satırların uzunluğunu belirlemek için buffer boyutu olarak kullanılıyor, standart bir güvenlik önlemi
    while (fgets(line, sizeof(line), file)) {
        int movie_id;
        sscanf(line, "%d", &movie_id);
        for (int i = 0; i < MAX_GENRES; i++) {  // ilk 4  ifadeyi geçiyoruz (id, film ismi,tarih, link)
            if (line[5 + i] == '1') {           //0 tabanlı döngüye sadık kalmaya çalışıyoruz.
                movie_genres[movie_id - 1][i] = 1;  // Film türlerini ilişkilendiriyoruz
            }
        }
    }

    fclose(file);
}

// kullanıcının en çok izlediği türden izlemediği filmleri öerir
void recommend_movies(int user_id, int favorite_genre, int recommend_count) { // öneri yapılacak kullanıcı, o kullanıcının favori türü ve kaç öneri yapılacağı alınır
    int recommended_movies[MAX_MOVIES];
    double recommended_ratings[MAX_MOVIES];
    int count = 0;

    // İzlenmeyen filmleri ve türününe göre öneri puanı ile doldur
    for (int i = 0; i < MAX_MOVIES; i++) {
        if (bipartite_matrix[user_id][i] == 0 && movie_genres[i][favorite_genre] == 1) { // kullanıcının izlemedikleri ve favori türe ait olan filmleri bulur
            recommended_movies[count] = i;
            //İzlenmemiş ve favori türe ait filmi, recommended_movies dizisine ekler.


            double genre_score = 0;
            int user_count = 0;
            for (int j = 0; j < MAX_USERS; j++) { //bütün kullanıcıları tarar
                if (bipartite_matrix[j][i] > 0) { // kullanıcı bu filmi izlemişse puanını tutar 
                    genre_score += bipartite_matrix[j][i];   //puanı ekler
                    user_count++; // kullanıcı sayısını arttırır ( en son ortalama ratingi bulmak için böyle yapıyor)
                }
            }

            // Ortalama rating hesapla (Puanları toplam kullanıcı sayısına bölüyoruz)
            if (user_count > 0) {
                genre_score /= user_count;
            }
            recommended_ratings[count] = genre_score;  // Puanı kaydet
            count++;
        }
    }

    // En yüksek ratingli filmleri sırala   (selection sortun büyükten küçüğe sıralanmış hali gibi)
    //filmlerin ratiblerini büyükten küçüğe sıralıyoruz yani
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (recommended_ratings[i] < recommended_ratings[j]) {
                int temp_movie = recommended_movies[i];
                double temp_rating = recommended_ratings[i];

                recommended_movies[i] = recommended_movies[j];
                recommended_ratings[i] = recommended_ratings[j];

                recommended_movies[j] = temp_movie;
                recommended_ratings[j] = temp_rating;
            }
        }
    }

    // İstenen sayıda film öner
    printf("\nIzlemediginiz en yuksek puanli %d film (Tur: %s):\n", recommend_count, genres[favorite_genre]);
    for (int i = 0; i < recommend_count && i < count; i++) {
        printf("Film ID: %d, Ortalama Puan: %.2f\n", recommended_movies[i] + 1, recommended_ratings[i]);
    }
}

// Rastgele Yürüyüş Fonksiyonu
void random_walk_recommendation(int start_movie, int steps, int user_id, int recommend_count) {
    int current_movie = start_movie;
    int visited_movies[MAX_MOVIES] = {0}; // Tüm filmler başlangıçta ziyaret edilmemiş olarak işaretleniyor (0)
    int recommendations = 0;

    for (int i = 0; i < steps; i++) {
        int next_movie = -1;
        int neighbors[MAX_MOVIES];
        int neighbor_count = 0;


    // Ziyaret edilmemiş filmleri komşu olarak belirliyoruz
        for (int j = 0; j < MAX_MOVIES; j++) {
            if (j != current_movie && visited_movies[j] == 0) {
                neighbors[neighbor_count++] = j;
            }
        }

        if (neighbor_count > 0) {
            next_movie = neighbors[rand() % neighbor_count];
            visited_movies[next_movie] = 1;
        }

        if (next_movie == -1) {
            break;
        }

        current_movie = next_movie;
    }

    printf("\nRastgele yuruyus sonucunda ziyaret edilen filmlerden oneriler:\n");
    for (int i = 0; i < MAX_MOVIES && recommendations < recommend_count; i++) {
        if (visited_movies[i] == 1 && bipartite_matrix[user_id][i] == 0) {
            printf("Film ID: %d\n", i + 1);
            recommendations++;
        }
    }
}

int main() {

    srand(time(NULL));

    int user_id, recommend_count, steps;
    int favorite_genre, max_count;

    // Dosya okuma işlemleri
    build_bipartite_matrix("u.data");  // Kullanıcı-film ilişkisini adjacency matrix ile oluşturuyoruz
    build_movie_genres("u.item");      // Film türlerini okuyarak ilişkileri bipartite_matrix ile eşleştirme
//film tür matrisi oluşturuyoruz

    // Kullanıcıdan hangi kullanıcı için öneri istediğini sor
    printf("Hangi kullanici icin oneri istiyorsunuz? (1 ile 943 arasi bir deger giriniz): ");
    scanf("%d", &user_id);
    

    // Kullanıcı ID'sini kontrol et
    if (user_id < 1 || user_id > MAX_USERS) {
        printf("Gecersiz kullanici ID'si. 1 ile 943 arasinda bir deger giriniz.\n");
        return 1;
    }

    // Kullanıcıdan kaç tane öneri istediğini sor
    printf("Kac film onerisi istiyorsunuz? ");
    scanf("%d", &recommend_count);

    printf("Rastgele yuruyus icin adim sayisini giriniz: ");
    scanf("%d", &steps);

    // Kullanıcının izlediği filmlerin türlerinr bakar her türden ne kadar film izlemiş saklar
    for (int i = 0; i < MAX_MOVIES; i++) {
        if (bipartite_matrix[user_id - 1][i] > 0) {  // Eğer kullanıcı bu filmi izlediyse
            for (int j = 0; j < MAX_GENRES; j++) {
                if (movie_genres[i][j] == 1) {
                    user_genre_count[user_id - 1][j]++;
                }
            }
        }
    }
    //user_genre_count global  

    // user_genre_count  bunu kullanarak her tür için tek tek bakar ve en çok izleneni bulur
    find_favorite_genre(user_id - 1, &favorite_genre, &max_count); 

    
    printf("\nKullanici %d en cok %s turunu izlemis (%d kez).\n", user_id, genres[favorite_genre], max_count);

    // Kullanıcının istedigi sayıda film önerme
    recommend_movies(user_id - 1, favorite_genre, recommend_count);
    // kullanıcının en çok izlediği türdeki tüm filmler bulunur (zaten izlenenler hariç)
    // bu her film için imdb puanı hesaplanır ve bu puanın en yüksek olduğu filmlerden başlanarak önerilir
    printf("\n");

    int start_movie = rand() % MAX_MOVIES; // rassal yürüyüş mesafesi için rastgelelik sağlasın diye
    random_walk_recommendation(start_movie, steps, user_id - 1, recommend_count);

    return 0;
}
