#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// 20200110 정태현
// linkstate 이용

#define MAX_NODES 100
#define INFINITY 1000000
#define TRUE 1
#define FALSE 0

typedef struct {
    int numNodes;
    int dist[MAX_NODES][MAX_NODES];
    int next[MAX_NODES][MAX_NODES];
} Network;

Network original; // original을 전역변수로 선언

// 문제없음
void read_network(Network *network, FILE *file) {
    int numNodes;
    fscanf(file, "%d", &numNodes);
    network->numNodes = numNodes;

    // Initialize distances and next nodes.
    for(int i = 0; i < numNodes; i++) {
        for(int j = 0; j < numNodes; j++) {
            if(i==j) {
                network->dist[i][j] = 0;
                network->next[i][j] = i;
            } else {
                network->dist[i][j] = INFINITY;
                network->next[i][j] = -1;
            }
        }
    }

    // Read edges.
    int i, j, d;
    while(fscanf(file, "%d %d %d", &i, &j, &d) == 3) {
        network->dist[i][j] = d;
        network->dist[j][i] = d;
        network->next[i][j] = i;  // 수정
        network->next[j][i] = j;  // 수정
    }
}

// 네트워크를 원본에 복사하는 것!!
void copy_network(Network *dest, Network *src) {
    dest->numNodes = src->numNodes;
    for(int i=0; i < src->numNodes; i++) {
        for(int j=0; j < src->numNodes; j++) {
            dest->dist[i][j] = src->dist[i][j];
            dest->next[i][j] = src->next[i][j];
        }
    }
}

// 라우팅 테이블 업데이트 하는 것!!
void update_routing_table(Network *network, int startNode) {
    int numNodes = network->numNodes;
    int *dist = (int*)malloc(numNodes*sizeof(int));
    int *visited = (int*)malloc(numNodes*sizeof(int));
    int *next = (int*)malloc(numNodes*sizeof(int));

    for(int i = 0; i < numNodes; i++) {
        dist[i] = original.dist[startNode][i];
        visited[i] = FALSE;
    }

    dist[startNode] = 0;          // start노드에서 start노드까지의 거리는 0
    visited[startNode] = TRUE;    // visited True 해줘야!
    next[startNode] = startNode;  // start 노드에서 start 노드를 가기 위한 next는 자기 자신 
    
    for(int i = 0; i < numNodes; i++){
        if(original.dist[startNode][i] != INFINITY){ // startNode와 직접적 이웃이라는 것!
            if(i != startNode){
                next[i] = i;
            }
        }
    }

    for(int i = 0; i < numNodes - 1; i++) {
        int min = INFINITY;
        int u = -1;
        for(int j = numNodes-1; j >= 0; --j) {
            if(!visited[j] && dist[j] <= min) {
                min = dist[j];
                u = j;
            }
        } // u가 추가될 인덱스!

        if(u == -1)
            break;

        visited[u] = TRUE; // u 방문!

        // 새로운 노드 visited 되고 나서 이제 dist 업데이트!!
        for(int v = 0; v < numNodes; v++) {
            if(!visited[v] && dist[u] + original.dist[u][v] < dist[v]) {
                dist[v] = dist[u] + original.dist[u][v];
                
                // 이 부분 더 날카롭게 분석해야!!
                if(next[u] == startNode){
                    next[v] = u; // next값 업데이트
                }else{
                    next[v] = next[u];
                }
            }
        }
    }

    // network에 반영!
    for(int i=0; i < numNodes; i++){
        network->dist[startNode][i] = dist[i];
        network->next[startNode][i] = next[i];
    }

    // 동적할당 해제
    free(dist);
    free(visited);
    free(next);
}

// outputfile에 라우팅 테이블 출력하는 함수!
void print_routing_tables(FILE* outputfile, Network *network) {
    int numNodes = network->numNodes;

    for(int i = 0; i < numNodes; i++) {
        for(int j = 0; j < numNodes; j++) {
            if(network->next[i][j] != -1 && network->dist[i][j] != INFINITY){
                fprintf(outputfile, "%d %d %d\n", j, network->next[i][j], network->dist[i][j]);
            }
        }
        fprintf(outputfile, "\n");
    }
}

// messagefile을 읽고 처리한 뒤 outputfile에 기록하는 함수 (문제 없음)
void process_and_send_message(FILE* messagefile, FILE* outputfile, Network* network) {
    int source, destination;
    char message[1024];
    
    while (fscanf(messagefile, "%d %d ", &source, &destination) != EOF) {
        fgets(message, 1024, messagefile); // get the rest of the line as the message
        
        // x부터 y까지 path가 존재하는지 체크
        if (network->dist[source][destination] == INFINITY) {
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
                current_node = network->next[current_node][destination];
                path[path_len++] = current_node;
            }
        
            // Print to output file
            fprintf(outputfile, "from %d to %d cost %d hops", source, destination, network->dist[source][destination]);
            for (int i = 0; i < path_len - 1; i++) {  // 마지막 destnation은 찍으면 안 되니까!
                fprintf(outputfile, " %d", path[i]);
            }

            fprintf(outputfile, " message %s", message);
        }
    }
    fprintf(outputfile, "\n"); // 마지막에 한칸 띄워줘야 함!
}

// 오리지널 업데이트 하는 함수
void original_update(int source, int destination, int new_cost){

    if (new_cost == -999) { // 길이 끊겼을 경우!
        // network->dist[i][j] = INFINITY;
        // network->next[i][j] = -1;
        original.dist[source][destination] = INFINITY;
        original.dist[destination][source] = INFINITY;
        
        original.next[source][destination] = -1;
        original.next[destination][source] = -1;

    }else{ // 길이 새로 생성 or 기존 길의 cost 변화 인 경우!
        // network->next[i][j] = i;  // 수정
        // network->next[j][i] = j;  // 수정
        original.dist[source][destination] = new_cost;
        original.dist[destination][source] = new_cost;

        original.next[source][destination] = source;
        original.next[destination][source] = destination;
    }

    // 원본 업데이트 끝!!!!!
}

// Main
int main(int argc, char *argv[]){
    if(argc != 4) {
        fprintf(stderr, "usage: linkstate topologyfile messagesfile changesfile\n");
        exit(1);
    }

    FILE* topologyfile = fopen(argv[1], "r");
    FILE* messagefile = fopen(argv[2], "r");
    FILE* changesfile = fopen(argv[3], "r");
    FILE* outputfile = fopen("output_ls.txt", "w");

    if(topologyfile == NULL || messagefile == NULL || changesfile == NULL || outputfile == NULL){
        fprintf(stderr, "Error: open input file.");
        exit(1);
    }

    Network network; // 네트워크 변수 선언
    read_network(&network, topologyfile); // 네트워크 구성
    
    // 오리지널에 원본 저장해놓기!
    copy_network(&original, &network);

    // STEP 0 
    for (int i = 0; i < network.numNodes; i++) { // 각 라우팅 테이블 구성하기
        update_routing_table(&network, i);
    }
    // 초기에 구성된 라우팅 테이블 파일에 복사
    print_routing_tables(outputfile, &network);

    // 메세지 파일 읽고 내용 처리!
    process_and_send_message(messagefile, outputfile, &network);

    // STEP 1
    int source, destination, new_cost;
    // rewind(changesfile);
    while (fscanf(changesfile, "%d %d %d", &source, &destination, &new_cost) != EOF) {
        
        original_update(source, destination, new_cost); // original 업데이트 완료!
        
        for (int i = 0; i < network.numNodes; i++) { // 각 라우팅 테이블 재구성하기
            update_routing_table(&network, i);
        }

        print_routing_tables(outputfile, &network); // 업데이트 된 라우팅 테이블 출력
        rewind(messagefile);
        process_and_send_message(messagefile, outputfile, &network); // 메세지 파일 읽고 처리
    }


    printf("Complete. Output file written to output_ls.txt.\n");

    // 파일 클로즈
    fclose(topologyfile);
    fclose(messagefile);
    fclose(changesfile);
    fclose(outputfile);

    return 0;
}


