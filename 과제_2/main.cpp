#include <stdio.h>
#include <stdlib.h>

// 20200110 정태현
// linkstate 이용

#define MAX_NODES 100
#define INFINITY 1000000
#define TRUE 1
#define FALSE 0

typedef struct {

    int cost[MAX_NODES][MAX_NODES];
    int parent[MAX_NODES][MAX_NODES]; // 해당 노드 기준에서 부모
    int via[MAX_NODES][MAX_NODES]; // start 기준에서 바로 다음
    int SPT[MAX_NODES]; // 탐색 서칭 순서를 담는 배열
    // 해당 인덱스 노드를 서칭 할 때 근본이 되는 것 배열
    int nodeRoot[MAX_NODES][MAX_NODES]; 

} Router;

typedef struct {
    int nodes;
    Router router[MAX_NODES];
} Network;

int distance[MAX_NODES]; /* 시작정점으로부터의 최단경로 거리 */
int found[MAX_NODES];    /* 방문한 정점 표시 */

Network original; // original 루트 선언!

void copyNetwork(Network* original, Network* src) {

    original->nodes = src->nodes; // nodes(라우터 개수) 복사

    // 라우터 별, cost, via 복사

    for(int i=0; i<original->nodes; i++){
        for(int j=0; j<original->nodes; j++){
            original->router[i].cost[i][j] = src->router[i].cost[i][j];
            original->router[i].via[i][j] = src->router[i].via[i][j];

            original->router[i].parent[i][j] = src->router[i].parent[i][j];
            original->router[i].nodeRoot[i][j] = src->router[i].nodeRoot[i][j];
            original->router[i].SPT[j] = src->router[i].SPT[j];
            // 싹 복사! 해놓기
        }
    }

}

// 네트워크 형성하는 함수
void read_network(Network* network, FILE* file) {
    int nodes;
    fscanf(file, "%d", &nodes);
    network->nodes = nodes; // nodes 업데이트

    for (int i = 0; i < network->nodes; i++) {
        for (int j = 0; j < network->nodes; j++) {
            if (i == j) {
                network->router[i].cost[i][j] = 0;
                network->router[i].via[i][j] = i;
            } else {
                network->router[i].cost[i][j] = INFINITY;
                network->router[i].via[i][j] = INFINITY;
            }
            network->router[i].SPT[j] = -1; // SPT를 처음에 -1로 쭉 초기화
        }
    }

    int source, destination, cost;
    while (fscanf(file, "%d %d %d", &source, &destination, &cost) != EOF) {
        network->router[source].cost[source][destination] = cost;
        network->router[destination].cost[destination][source] = cost;
        network->router[source].via[source][destination] = source;
        network->router[destination].via[destination][source] = destination;

        // start의 부모는 start 자기 자신
        network->router[source].parent[source][destination] = source;
        network->router[destination].parent[destination][source] = destination;

        // destination의 부모는 start;
        network->router[source].parent[destination][source];
        network->router[destination].parent[source][destination];

        // start의 rootNode는 start 자기 자신
        network->router[source].nodeRoot[source][destination] = source;
        network->router[destination].nodeRoot[destination][source] = destination;

        // destination의 루트는 start;
        network->router[source].nodeRoot[destination][source];
        network->router[destination].nodeRoot[source][destination];

    }
}

// 다익스트라 알고리즘 helper function
int choose(int distance[], int n, int found[]){
    int i, min, minpos;
    min = INFINITY;
    minpos = -1;
    for(i = n-1 ; i >= 0; i--){
        if(distance[i] <= min && !found[i]){
            min = distance[i];
            minpos = i;
        }
    }
    return minpos;
}

// 다익스트라 알고리즘으로 라우팅 테이블 업데이트~
void update_routing_table(Network* network, int start) {

    int i, j;
    int min_index;

    // distance와 found 초기화!!
    for (i = 0; i < network->nodes; i++) {
        found[i] = FALSE;
        distance[i] = original.router[start].cost[start][i];
    }

    found[start] = TRUE; // start 부분 found 등록
    distance[start] = 0; // distance 당연히 0으로 초기화

    int orderIndex = 0;
    network->router[start].SPT[orderIndex] = start; // 해당 라우터의 traceOrder 배열에 넣기
    orderIndex++;

    // 다익스트라 알고리즘 수행
    for (i = 0; i < network->nodes - 1; i++) {
        min_index = choose(distance, network->nodes, found);
        found[min_index] = TRUE; // found 추가

        // traceOrder 업데이트
        network->router[start].SPT[orderIndex] = min_index;
        orderIndex++; // SPT에 들어와 있는 원소 수를 나타내게!

        // 여기부분 수정하기!!!!!(다시 검토 ㄱㄱ)
        for (j = 0; j < network->nodes; j++) {  // original.router[start].cost[min_index][j]
            if(!found[j]){
                if(distance[min_index] + original.router[min_index].cost[min_index][j] < distance[j]){
                    distance[j] = distance[min_index] + original.router[start].cost[min_index][j];
                    network->router[start].cost[start][j] = distance[j];
                    network->router[start].via[start][j] = min_index;
                }
            }
        }
        
        for(i=0; i<network->nodes; i++){
            network->router[start].cost[start][i] = distance[i];
        }

    }
}

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

int main(int argc, char *argv[]){

    if(argc != 4) {
        fprintf(stderr, "usage: linkstate topologyfile messagesfile changesfile\n");
        exit(1);
    }

    FILE* topologyfile = fopen(argv[1], "r");
    FILE* messagefile = fopen(argv[2], "r");
    FILE* changesfile = fopen(argv[3], "r");
    
    // 출력할 파일 Output file 열기
    FILE* outputfile = fopen("output_ls.txt", "w");

    // fopen 예외 처리
    if(topologyfile == NULL || messagefile == NULL || changesfile == NULL || outputfile == NULL){
        fprintf(stderr, "Error: open input file.");
        exit(1);
    }


    /* STEP 0 */
    Network network; // 네트워크 구성
    read_network(&network, topologyfile);

    /* 네트워크 만들고 원본 복사 해놓은 다음 그 초기 원본을 기준으로!!!! 
    각 노드의 라우팅 테이블 작성해야 함!!! 깨달음!! */
    copyNetwork(&original, &network); // network에 초기화 된 원본을 original에 복사!! 
    

    // 라우팅 테이블 구성!
    for (int i = 0; i < network.nodes; i++) {
        update_routing_table(&network, i);
    }

    // 출력 (잘 되었나 확인 차)
    print_routing_tables(outputfile, &network);

    // {{0, 7, INF, INF, 3, 10, INF},
    //     {7, 0, 4, 10, 2, 6, INF},
    //     {INF, 4, 0, 2, INF, INF, INF},
    //     {INF, 10, 2, 0, 11, 9, 4}, 
    //     {3, 2, INF, 11, 0, INF, 5},
    //     {10, 6, INF, 9, INF, 0, INF},
    //     {INF, INF, INF, 4, 5, INF, 0}}



    // 프로그램 마지막 메시지!
    printf("Complete. Output file written to output_ls.txt.\n");

    return 0;
}