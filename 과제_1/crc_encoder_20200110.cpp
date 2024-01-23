#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 20200110 정태현
// crc_encoder_20200110.cc

// crc를 이용해 dataword를 codeword로 바꿔서 반환하는 함수
char* perform_crc(const char* input, const char* divisor);

// 1바이트 크기의 바이너리 데이터를 길이가 8인 char형 문자열에 이진수로 변환하는 함수
void byte_to_binary_string(unsigned char byte, char *binary_str); 

// 1바이트 크기의 바이너리 데이터를 길이가 4인 char형 배열 두개로 쪼개어 이진수로 담는 함수 
void split_byte_into_two_arrays(unsigned char byte, char *arr1, char *arr2); 

// 바이너리 문자열을 바이너리 코드로 변환하는 함수
void binaryStringToBinary(const char *binaryString, unsigned char *output, int output_size); 


// main
int main(int argc, char *argv[]){

    if (argc != 5) {
        fprintf(stderr, "usage: %s input_file output_file generator dataword_size\n", argv[0]);
        return 1;
    }

    FILE* input_file = fopen(argv[1], "rb");
    FILE* output_file = fopen(argv[2], "wb");
    const char* generator = argv[3];  // divisor == generator
    int dataword_size = atoi(argv[4]);

    if (input_file == NULL) {
        printf("input file open error.\n");
        exit(1);
    }

    if (output_file == NULL) {
        printf("output file open error.\n");
        exit(1);
    }

    // 데이터 워드 길이 예외처리
    if (dataword_size != 4 && dataword_size != 8) {
        fprintf(stderr, "dataword size must be 4 or 8.\n");
        exit(1);
    }

    // 제너레이터(divisor)의 길이와 dataword의 사이즈에 따라 padding이 결정됨!
    // 따라서 우린 padding 사이즈를 앞의 두 조건을 통해 구현해야 함
    int generator_size = strlen(generator);
    int totalLen = dataword_size + generator_size - 1; // totalLen이 codeword의 길이
   
    char* codeword;  // padding 까지 붙인 최종적인 codeword 가르킬 포인터
    int padding_size;  // codeword에 추가한 패딩의 길이 담는 변수

    int whileCount = 0;  // padding 값 넣어주는 것 관련해서 만든 변수

    while (!feof(input_file)) { // 파일 끝까지 계속 돌리기

        unsigned char buffer;
        size_t read_count = fread(&buffer, 1, 1, input_file); // input_file에서 1바이트 1개를 읽어와 buffer 담기
        if (read_count != 1) { 
            break;
        }

        if (dataword_size == 4) {
            int remainder = (totalLen * 2) % 8; // 8로 나눈 나머지 

            char arr1[5] = {0};
            char arr2[5] = {0};
            split_byte_into_two_arrays(buffer, arr1, arr2);

            // arr1의 1차 codeword 만들기
            char* temp1 = perform_crc(arr1, generator);

            // arr2의 1차 codeword 만들기
            char* temp2 = perform_crc(arr2, generator);
        
            if (remainder == 0) { // padding 필요없음
                codeword = (char*)malloc(sizeof(char) * (strlen(temp1) + strlen(temp2) + 1));
                strcpy(codeword, temp1);
                strcat(codeword, temp2);
                free(temp1);
                free(temp2);
                padding_size = 0;

            } else {  // padding 필요
                codeword = (char*)malloc(sizeof(char)*(totalLen * 2 + (8 - remainder) + 1));
                if(codeword == NULL){ 
                    printf("메모리 동적할당 에러\n");
                    exit(1);
                }
                padding_size = 8 - remainder;
                for (int i = 0; i < padding_size; i++){ // padding 0으로 채우기       
                    codeword[i] = '0';
                }
                
                strcpy(codeword + padding_size, temp1);
                strcat(codeword, temp2);
                free(temp1);
                free(temp2);
            }

        }else if (dataword_size == 8) {
            char dataword_bits[9] = {0};
            byte_to_binary_string(buffer, dataword_bits); // dataword_bits에 8비트 담김 맨 뒤는 NULL문자
            char* ptr = perform_crc(dataword_bits, generator); // 1차 codeword 완성해서 나옴 
            
            if (totalLen % 8 == 0) {  // 이러면 padding 필요 없음
                codeword = (char*)malloc(sizeof(char)*(totalLen + 1));
                strcpy(codeword, ptr);
                free(ptr);  // 최종본 만들었으니 1차 내용 담았던 malloc 해제 시켜주기
                padding_size = 0;  // padding 사이즈 정보 저장

            }else{
                int box = totalLen % 8;  
                codeword = (char*)malloc(sizeof(char)*(totalLen + (8 - box) + 1));  // padding 입힌 최종 codeword
                for(int i = 0; i < 8-box; i++){ // padding 입히기
                    codeword[i] = '0';
                }
                strcpy(codeword + (8 - box), ptr); // 패딩 뒤에 이어서 붙이기~
                
                free(ptr);  // 최종본 만들었으니 1차 내용 담았던 malloc 해제 시켜주기
                padding_size = 8-box;  // padding 사이즈 정보 저장
            }
        }

        // 이제 padding까지 입힌 최종적인 codeword가 나왔으니 바이너리 파일에 보내줘야 함
        // padding 사이즈하고 해당 codeword문자열 배열하고의 정보를 바이너리 파일에다가 보내줘야 함
        
        if(whileCount == 0){
            unsigned char paddingAscii = padding_size + '0'; // padding size를 아스키 코드로 바꿔서 저장.
            fwrite(&paddingAscii, sizeof(unsigned char), 1, output_file); // padding 정보 입력
            whileCount++;
            // 제일 처음에만 padding이 몇비트인지 써 줘야 하므로 이렇게 if문으로 처리
        }
        
        // padding까지 붙은 codeword 정보를 총 몇 바이트로 바꿔야할지 길이 반환
        int binaryDataLength = strlen(codeword) / 8;  
        unsigned char* codewordBinary = (unsigned char*)malloc(sizeof(unsigned char) * binaryDataLength);
        
        // 바이너리 문자열을 바이너리 코드로 변환
        binaryStringToBinary(codeword, codewordBinary, binaryDataLength); 

        // output_file 파일에 codeword 정보 쓰기!!!
        fwrite(codewordBinary, sizeof(unsigned char), binaryDataLength, output_file); // codeword 데이터 넣기

        free(codeword); // codeword 메모리 동적 해제
        free(codewordBinary); // codewordBinary 메모리도 동적 해제
    
    }

    // 파일 입출력 종료
    fclose(input_file);
    fclose(output_file);

    return 0;
}

char* perform_crc(const char* input, const char* divisor) {

    int input_len = strlen(input);
    int divisor_len = strlen(divisor);

    // Append zeroes to the input
    //char* appended_input = (char*)malloc(sizeof(char)*(input_len + divisor_len - 1));
    char appended_input[input_len + divisor_len - 1];
    strcpy(appended_input, input);
    for (int i = input_len; i < input_len + divisor_len; i++) {
        appended_input[i] = '0';
    }
    appended_input[input_len + divisor_len - 1] = '\0';

    // XOR연산 수행 
    for (int i = 0; i <= input_len - 1; i++) {

        if (appended_input[i] == '1') {
            for (int j = 0; j < divisor_len; j++) {
                appended_input[i + j] = (appended_input[i + j] == divisor[j]) ? '0' : '1';
            }
        }
    }

    // Copy remainder to output
    char* remainder = (char*)malloc(sizeof(char) * (divisor_len));
    strncpy(remainder, appended_input + input_len, divisor_len - 1);
    remainder[divisor_len - 1] = '\0';
    

    // 최종 codeword 담을 그릇 만들기
    int codeword_len = input_len + divisor_len;
    char* codeword = (char*)malloc(codeword_len + 1);
    
    if(codeword == NULL){
        printf("동적할당 에러\n");
        exit(1);
    }

    strcpy(codeword, input);
    strcpy(codeword + input_len, remainder);
    codeword[codeword_len] = '\0';

    // 이제 리메인더 삭제
    free(remainder);

    // 최종 
    return codeword;
}

void byte_to_binary_string(unsigned char byte, char *binary_str) {
    for (int i = 0; i < 8; i++) {
        binary_str[7 - i] = (byte & (1 << i)) ? '1' : '0';
    }
}

// 오류 없어보임
void split_byte_into_two_arrays(unsigned char byte, char *arr1, char *arr2) {
    for (int i = 0; i < 4; i++) {
        arr1[i] = (byte & (1 << (7 - i))) ? '1' : '0';
        arr2[i] = (byte & (1 << (3 - i))) ? '1' : '0';
    }
}

void binaryStringToBinary(const char *binaryString, unsigned char *output, int output_size) {
    for (int i = 0; i < output_size; ++i) {
        unsigned char byte = 0;
        for (int j = 0; j < 8; ++j) {
            byte = (byte << 1) | (binaryString[i * 8 + j] - '0');
        }
        output[i] = byte;
    }
}