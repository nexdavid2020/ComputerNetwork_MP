#include <stdio.h>
#include <stdlib.h>

// Distance Vector 이용
// 20200110 정태현
// (테스트 케이스 다 통과 -> 완성) 진짜 굳굳!!

#define MAX_NODES 100
#define INFINITY 1000000

typedef struct {
    int cost[MAX_NODES][MAX_NODES];
    int via[MAX_NODES][MAX_NODES];
    int neighbors[MAX_NODES];  // 이웃 노드에 대한 정보를 추가합니다.
    int num_neighbors;  // 이웃 노드의 수를 추가합니다.
} Router;

typedef struct {
    int nodes;
    Router router[MAX_NODES];
} Network;

Network original;
int** ptr; // 길 삭제 할 때 쓸 2차원 배열 만들어야!


void copyNetwork(Network* original, Network* src) {

    original->nodes = src->nodes; // nodes(라우터 개수) 복사
    // 라우터 별, 이웃 숫자 복사
    for(int i=0; i<original->nodes; i++){
        original->router[i].num_neighbors = src->router[i].num_neighbors;
    } 

    // 라우터 별, 이웃들의 1차원 배열 값 복사
    for(int i=0; i<original->nodes; i++){
        for(int j=0; j<MAX_NODES; j++){
            original->router[i].neighbors[j] = src->router[i].neighbors[j];
        }
    } 

    // 라우터 별, cost, via 복사
    for(int k=0; k<original->nodes; k++){
        for(int i=0; i<MAX_NODES; i++){
            for(int j=0; j<MAX_NODES; j++){
                original->router[k].cost[i][j] = src->router[k].cost[i][j];
                original->router[k].via[i][j] = src->router[k].via[i][j];
            }
        }
    }
}

// neighbors 배열 정렬하는 함수
void sort_neighbors(Router* router) {
    for (int i = 0; i < router->num_neighbors - 1; i++) {
        for (int j = 0; j < router->num_neighbors - i - 1; j++) {
            if (router->neighbors[j] > router->neighbors[j + 1]) {
                int temp = router->neighbors[j];
                router->neighbors[j] = router->neighbors[j + 1];
                router->neighbors[j + 1] = temp;
            }
        }
    }
}

// 네트워크 형성하는 함수
void read_network(Network* network, FILE* file) {
    int nodes;
    fscanf(file, "%d", &nodes);
    network->nodes = nodes;

    for (int i = 0; i < nodes; i++) {
        for (int j = 0; j < nodes; j++) {
            if (i == j) {
                network->router[i].cost[i][j] = 0;
                network->router[i].via[i][j] = i;
            } else {
                network->router[i].cost[i][j] = INFINITY;
                network->router[i].via[i][j] = INFINITY;
            }
        }
        network->router[i].num_neighbors = 0;  // 이웃 노드 수 초기화
    }

    int source, destination, cost;
    while (fscanf(file, "%d %d %d", &source, &destination, &cost) != EOF) {
        network->router[source].cost[source][destination] = cost;
        network->router[destination].cost[destination][source] = cost;
        network->router[source].via[source][destination] = destination;
        network->router[destination].via[destination][source] = source;

        // 이웃 노드 정보 추가, 이웃 개수도 하나 증가시킴
        network->router[source].neighbors[network->router[source].num_neighbors++] = destination;
        network->router[destination].neighbors[network->router[destination].num_neighbors++] = source;
    }

    // 각 라우터 별 이웃 정보 정렬
    for(int i=0; i<network->nodes; i++){
        sort_neighbors(&network->router[i]);
    }
}

// 메세지 전송 기록을 읽고 처리한 뒤 output 파일에 기록하는 함수 (문제 없음)
void process_and_send_message(FILE* messagefile, FILE* outputfile, Network* network) {
    int source, destination;
    char message[1024];
    
    while (fscanf(messagefile, "%d %d ", &source, &destination) != EOF) {
        fgets(message, 1024, messagefile); // get the rest of the line as the message
        
        // x부터 y까지 path가 존재하는지 체크
        if (network->router[source].cost[source][destination] == INFINITY) {
            // path 가 존재하지 않는다면, 에러메시지 출력하기 
            fprintf(outputfile, "from %d to %d cost infinite hops unreachable message %s", source, destination, message);
        } else { 
            // Path가 존재한다면!
            // Now find the path
            int path[MAX_NODES]; //path 저장하는 배열
            int current_node = source;
            int path_len = 0;
            
            // Add source to path
            path[path_len++] = current_node;
        
            // Follow the path to the destination
            while (current_node != destination) {
                current_node = network->router[current_node].via[current_node][destination];
                path[path_len++] = current_node;
            }
        
            // Print to output file
            fprintf(outputfile, "from %d to %d cost %d hops", source, destination, network->router[source].cost[source][destination]);
            for (int i = 0; i < path_len - 1; i++) {  // 마지막 destnation은 찍으면 안 되니까!
                fprintf(outputfile, " %d", path[i]);
            }

            fprintf(outputfile, " message %s", message);
        }
    }
    fprintf(outputfile, "\n"); // 마지막에 한칸 띄워줘야 함!
}

// 각 라우터의 완성된 라우팅 테이블을 outputfile에 출력하는 함수 (문제 없음)
void print_routing_tables(FILE* outputfile, Network* network) {
    for (int i = 0; i < network->nodes; i++) { // 각 라우터에 대해
        for (int j = 0; j < network->nodes; j++) { // 모든 목적지에 대해
            if(network->router[i].via[i][j] != INFINITY && network->router[i].cost[i][j] != INFINITY){
                fprintf(outputfile, "%d %d %d\n", j, network->router[i].via[i][j], network->router[i].cost[i][j]);
            }
        }
        fprintf(outputfile, "\n"); // 라우터 사이에 빈 줄 삽입
    } 
}

// 라우팅 테이블 업데이트 하는 함수
void update_routing_table(Network* network) {
    int changes;
    do {
        changes = 0;  // 변화 없음으로 초기화

        for(int i = 0; i < network->nodes; i++){
            Router* r = &network->router[i];

            for(int j = 0; j < r->num_neighbors; j++){
                int neighbor = r->neighbors[j];  // 이웃 인덱스 추출!
                for(int k = 0; k < network->nodes; k++){
                    // 변화가 발생했는지를 감지하는 부분
                    int new_cost = r->cost[i][neighbor] + network->router[neighbor].cost[neighbor][k];
                    if(r->cost[i][k] > new_cost ||
                        (r->cost[i][k] == new_cost && r->via[i][k] > r->via[i][neighbor])){
                        r->cost[i][k] = new_cost;
                        r->via[i][k] = r->via[i][neighbor];
                        changes += 1;  // 변화 발생
                    }
                }
            }
        }

    } while(changes);  // 변화가 발생하지 않을 때까지 반복
}

// 라우터랑 어떤 라우터랑 현재 이웃관계인지 아닌지 체크하는 함수
int is_neighbor(Router* router, int neighbor_id) {
    for (int i = 0; i < router->num_neighbors; i++) {
        if (router->neighbors[i] == neighbor_id) {
            return 1;  // 이미 이웃이면 1 반환
        }
    }
    return 0;  // 이웃이 아니면 0 반환
}

// 이웃 추가 함수(apply_changes_and_update 함수의 helper function) 정렬은 안 했음
void add_neighbor(Router* router, int neighbor_id) {
    router->neighbors[router->num_neighbors] = neighbor_id;  // 새 이웃 추가
    router->num_neighbors++;  // 이웃의 수를 하나 늘립니다.
}

// 이웃 제거 함수 (apply_changes_and_update 함수의 helper function)
void remove_neighbor(Router* router, int neighbor_id) {
    int index = -1;
    for (int i = 0; i < router->num_neighbors; i++) {
        if (router->neighbors[i] == neighbor_id) {
            index = i;
            break;
        }
    }

    if (index != -1) {  // 해당 이웃을 찾았다면
        for (int i = index; i < router->num_neighbors - 1; i++) {
            router->neighbors[i] = router->neighbors[i + 1];  // 배열을 앞으로 한 칸씩 당겨서 덮어쓰기
        }
        router->num_neighbors--;  // 이웃의 수를 하나 줄입니다.
    }
}

// path_search의 헬퍼 함수
int checkConsecutive(int path[], int length, int num1, int num2) {
    for(int i = 0; i < length - 1; i++) {
        if((path[i] == num1 && path[i+1] == num2) || (path[i] == num2 && path[i+1] == num1)) {
            return 1;
        }
    }
    return 0;
}

void path_search(Network* network, int source, int destination){
    int path[MAX_NODES];
    int current_node;
    int path_len;
    int end;
    // i 부터 j까지의 경로가 있을 때 
    for(int i=0; i < network->nodes; i++){
        for(int j=0; j < network->nodes; j++){
            
            current_node = i;
            path_len = 0;
            path[path_len++] = current_node;
            end = j;
            
            // Follow the path to the destination
            while (current_node != end) {
                current_node = network->router[current_node].via[current_node][end];
                path[path_len++] = current_node;
            }

            if(checkConsecutive(path, path_len, source, destination) == 1){
                // 이 함수 반환값이 1이 나오면 해당 경로가 들어가 있던 것!!
                ptr[i][j] = 1; // i로부터 j까지의 경로 중 삭제된 경로가 있었다는 것!!
            }else{
                // 이러면 없는 것!!
                ptr[i][j] = 0;
            }

        }
    }

}

// change.txt 파일 읽고 처리하는 것!
void apply_changes_and_update(FILE* changesfile, FILE* outputfile, FILE* messagefile, Network* network) {
    
    int source, destination, new_cost;
    rewind(changesfile);
    while (fscanf(changesfile, "%d %d %d", &source, &destination, &new_cost) != EOF) {
        
        if (new_cost == -999) { // source 부터 destination 까지의 도로가 끊긴다면
            // 이제 기존에 다른 라우터들끼리 에서도 1과 3의 직선 도로를 이용했던 애들의 라우터도 다 다시 반영해야함
            // 여기가 제일 중요!!!
            
            // (추가) original에서 source 부터 destination 까지의 도로 끊어놔야 함!
            original.router[source].cost[source][destination] = INFINITY;
            original.router[destination].cost[destination][source] = INFINITY;
            remove_neighbor(&original.router[source], destination); // 이웃제거
            remove_neighbor(&original.router[destination], source); // 이웃제거
            // 어차피 source랑 destination이랑 이웃아니었으면 아무일 일어나지 않음
            original.router[source].via[source][destination] = INFINITY; // via 제거
            original.router[destination].via[destination][source] = INFINITY; // via 제거
            // (추가 여기까지) 여기까지가 original에 반영!!

            // 이전 루트들 다 따기!!
            path_search(network, source, destination); // 이제 본격적인 network에서의 길 따기
            // 이 함수 끝나면 ptr에 source-destination or destination-source 쓴 길들 다 담겨 있음!
            // ptr[i][j]가 1이면 이 경로도 다시 원래대로 돌려야 한다는 것이고, 0 이면 상관 없음
            
            for(int i=0; i < network->nodes; i++){
                for(int j=0; j < network->nodes; j++){
                    if(ptr[i][j] == 1){ // 바꿔줘야하는 경로
                        network->router[i].cost[i][j] = original.router[i].cost[i][j]; // 원래 걸로 바꿔주기
                        network->router[i].via[i][j] = original.router[i].via[i][j]; // 원래 걸로 바꿔주기
                    }
                }
            }

            // 현재 network에 source와 destination 사이에 마무리 처리
            network->router[source].cost[source][destination] = INFINITY;
            network->router[destination].cost[destination][source] = INFINITY;
            network->router[source].via[source][destination] = INFINITY; // '다음'을 없다로 바꿔야 함
            network->router[destination].via[destination][source] = INFINITY; // '다음'을 없다로 바꿔야 함
            remove_neighbor(&network->router[source], destination);
            remove_neighbor(&network->router[destination], source);
        
        } else { // source 부터 destination 까지의 도로의 cost가 수정되거나 새로 생긴다면!
            
            // (추가) original 반영
            if(is_neighbor(&original.router[source], destination) == 1){ // 기존에 있는 점이라면!
                original.router[source].cost[source][destination] = new_cost;
                original.router[destination].cost[destination][source] = new_cost;
                original.router[source].via[source][destination] = destination; // via 갱신 
                original.router[destination].via[destination][source] = source; // via 갱신
            }else{ // 새로 추가된 다이렉트 점이라면!
                original.router[source].cost[source][destination] = new_cost; 
                original.router[destination].cost[destination][source] = new_cost;
                original.router[source].via[source][destination] = destination; // via 갱신 
                original.router[destination].via[destination][source] = source; // via 갱신
                add_neighbor(&network->router[source], destination); // 이웃 추가
                add_neighbor(&network->router[destination], source); // 이웃 추가
            }
            // (추가 여기까지 original 관련하여)

            // 현재 네트워크 처리
            if(is_neighbor(&network->router[source], destination) == 1){   // 이미 이웃이라면
                network->router[source].cost[source][destination] = new_cost; // cost 갱신
                network->router[destination].cost[destination][source] = new_cost; // cost 갱신
                network->router[source].via[source][destination] = destination; // via 갱신 
                network->router[destination].via[destination][source] = source; // via 갱신
            }else{                                                        // 이웃이 아니었다면!!
                network->router[source].cost[source][destination] = new_cost; // cost 갱신
                network->router[destination].cost[destination][source] = new_cost; // cost 갱신
                network->router[source].via[source][destination] = destination; // via 갱신
                network->router[destination].via[destination][source] = source; // via 갱신
                add_neighbor(&network->router[source], destination); // 이웃 추가
                add_neighbor(&network->router[destination], source); // 이웃 추가
            }

        }
        
        // 테이블 업데이트 하기 전에 두 라우터 이웃 정보 오름차순으로 정렬
        sort_neighbors(&network->router[source]);
        sort_neighbors(&network->router[destination]);

        // 업데이트 하고 그거 output 파일에 출력
        update_routing_table(network);
        print_routing_tables(outputfile, network);

        rewind(messagefile);
        process_and_send_message(messagefile, outputfile, network);
        // fprintf(outputfile, "\n");
    }
}

int main(int argc, char *argv[]){

    if(argc != 4) {
        fprintf(stderr, "usage: distvec topologyfile messagesfile changesfile\n");
        exit(1);
    }

    FILE* topologyfile = fopen(argv[1], "r");
    FILE* messagefile = fopen(argv[2], "r");
    FILE* changesfile = fopen(argv[3], "r");
    
    // 출력할 파일 Output file 열기
    FILE* outputfile = fopen("output_dv.txt", "w");
    
    // fopen 예외 처리
    if(topologyfile == NULL || messagefile == NULL || changesfile == NULL || outputfile == NULL){
        fprintf(stderr, "Error: open input file.");
        exit(1);
    }

    /* STEP 0 */
    Network network; // 네트워크 구성
    read_network(&network, topologyfile);

    // 초기 세팅을 original에 복사해놓기!!!! (매우 중요) 
    copyNetwork(&original, &network);  

    // 길 삭제 할 때 쓸 ptr 동적할당!
    ptr = (int**)malloc(sizeof(int*) * network.nodes);
    for(int i=0; i<network.nodes; i++){
        ptr[i] = (int*)malloc(sizeof(int) * network.nodes);
    }

    // ptr 전체 다 0으로 초기화
    for(int i=0; i<network.nodes; i++){
        for(int j=0; j<network.nodes; j++){
            ptr[i][j] = 0;
        }
    }

    // 첫 라우팅 테이블 업데이트!!
    update_routing_table(&network);
    
    // 초기화된 라우팅 테이블 Output 파일에 출력하기
    print_routing_tables(outputfile, &network);

    // 이제 message 파일 읽어와서 Output 처리해야 함!
    process_and_send_message(messagefile, outputfile, &network);
    // fprintf(outputfile, "\n"); // 띄어쓰기 하나 추가

    /* STEP 1 */
    apply_changes_and_update(changesfile, outputfile, messagefile, &network);    

    // 파일 클로즈
    fclose(topologyfile);
    fclose(messagefile);
    fclose(changesfile);
    fclose(outputfile);

    //동적할당 해제
    for(int i=0; i<network.nodes; i++){
        free(ptr[i]);
    }
    free(ptr);

    // 마지막에 출력할 메시지 
    printf("Complete. Output file written to output_dv.txt.\n");

    return 0;
}