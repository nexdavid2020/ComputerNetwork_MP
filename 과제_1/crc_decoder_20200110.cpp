#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 20200110 정태현
// crc_decoder_20200110.cc

// input: binary 데이터 가진 배열, input_size 배열 길이, output char형 문자열 배열 주소
void binaryToBinaryString(const unsigned char *input, int input_size, char *output) {
    for (int i = 0; i < input_size; ++i) {
        unsigned char byte = input[i];
        for (int j = 0; j < 8; ++j) {
            output[i * 8 + (7 - j)] = '0' + ((byte >> j) & 1);
        }
    }
    output[input_size * 8] = '\0';
}

// 오류 체킹하는 decoder 함수
int crc_decoder(char* temp, const char* divisor) {
    int codeword_length = strlen(temp);
    int divisor_length = strlen(divisor);

    // 복사용 만들기
    char* codeword = (char*)malloc(sizeof(char)*codeword_length + 1);
    strcpy(codeword, temp);

    for (int i = 0; i <= codeword_length - divisor_length; i++) {
        if (codeword[i] == '1') {
            for (int j = 0; j < divisor_length; j++) {
                codeword[i + j] = codeword[i + j] == divisor[j] ? '0' : '1';
            }
        }
    }

    for (int i = codeword_length - divisor_length + 1; i < codeword_length; i++) {
        if (codeword[i] != '0') {
            return 1; // 오류 존재하는 경우 1 반환
        }
    }

    free(codeword);

    return 0; // 오류가 없는 경우 0 반환
}

// 이 함수가 문제 아님(보류)
// 바이너리 문자열을 바이너리 코드로 변환하는 함수
void binaryStringToBinary(const char* binaryString, unsigned char* output, int output_size) {
    for (int i = 0; i < output_size; ++i) {
        unsigned char byte = 0;
        for (int j = 0; j < 8; ++j) {
            byte = (byte << 1) | (binaryString[i * 8 + j] - '0');
        }
        output[i] = byte;
    }
    
}

// main
int main(int argc, char *argv[]){

    if (argc != 6) {
        fprintf(stderr, "usage: %s input_file output_file generator dataword_size\n", argv[0]);
        return 1;
    }

    FILE* input_file = fopen(argv[1], "rb");
    FILE* output_file = fopen(argv[2], "wb");
    FILE* result_file = fopen(argv[3], "w");
    const char* generator = argv[4];      // divisor == generator
    int dataword_size = atoi(argv[5]);

    if (input_file == NULL) {
        printf("input file open error.\n");
        exit(1);
    }

    if (output_file == NULL) {
        printf("output file open error.\n");
        exit(1);
    }

    if(result_file == NULL){
        printf("result file open error.\n");
        exit(1);
    }

    // 데이터 워드 길이 예외처리
    if (dataword_size != 4 && dataword_size != 8) {
        fprintf(stderr, "dataword size must be 4 or 8.\n");
        exit(1);
    }

    // 제너레이터(divisor) 길이
    int generator_size = strlen(generator);

    // 첫 비트 긁어와서 패딩 사이즈 구하기
    unsigned char buffer;
    size_t read_count = fread(&buffer, 1, 1, input_file);  // input_file에서 1바이트 1개를 읽어와 buffer 담기
    if(read_count != 1){
        printf("파일에서 데이터를 읽는데 실패1\n");
    }
    int padding_size = buffer - '0';  // 읽어온 padding size 아스키 코드값을 int형으로 바꿔주기  

    // result_file 파일에 담을 값 저장 변수들
    int codeWordCount = 0;
    int errorCount = 0;


    while (!feof(input_file)) { // 파일 끝까지 계속 돌리기

        
        if(dataword_size == 4){

            int totalLen = (dataword_size + generator_size - 1) * 2 + padding_size;
            char* binaryString = (char*)malloc(sizeof(char) * totalLen + 1);

            int byteBoxSize = totalLen / 8;
            unsigned char* byteBox = (unsigned char*)malloc(sizeof(char)* byteBoxSize);

            // 한 덩어리 만큼 긁어오기
            read_count = fread(byteBox, 1, byteBoxSize, input_file);
            
            if(read_count != byteBoxSize){
                if(feof(input_file)){
                    break;
                }else{
                    printf("파일에서 데이터를 읽는데 실패2\n");
                }
            }
            
            codeWordCount += 2; // 한 덩어리에서 codeword 두개 나오는 것이니 카운트 2 증가

            // 긁어온 바이너리 코드를 바이너리 문자열로 바꿔주기 -> binaryString에 담김
            binaryToBinaryString(byteBox, byteBoxSize, binaryString);
            
            // realCodeWord에는 padding을 제외한 바이너리 문자열이 담김
            int realCodeWordSize = totalLen - padding_size;
            char* realCodeWord = (char*)malloc(sizeof(char) * (realCodeWordSize + 1));

            int j = padding_size;
            for(int i = 0; i < realCodeWordSize; i++, j++){
                realCodeWord[i] = binaryString[j];
            }

            // 동적할당 해제할 것 해주기
            free(binaryString);
            free(byteBox);

            // padding 제거한 상황에서 이제 반반 씩 쪼개야
            char* arr1 = (char*)malloc(sizeof(char) * (realCodeWordSize / 2) + 1);
            int arr1Len = (realCodeWordSize / 2); // strlen(arr1)

            char* arr2 = (char*)malloc(sizeof(char) * (realCodeWordSize / 2) + 1);
            int arr2Len = (realCodeWordSize / 2);

            strncpy(arr1, realCodeWord, arr1Len);
            arr1[arr1Len] = '\0';   // 끝에 널문자 찍어주기

            strncpy(arr2, realCodeWord + arr1Len, arr2Len);
            arr2[arr2Len] = '\0';   // 끝에 널문자 찍어주기

            free(realCodeWord);     // 얘도 동적할당 해제

            
            // 오류 체크하기
            int opt1, opt2;
            opt1 = crc_decoder(arr1, generator);
            opt2 = crc_decoder(arr2, generator);

            if(opt1 == 0 && opt2 == 0){ // 오류 없는 것 
                
            }else if(opt1 == 0 && opt2 != 0){  // 오류 하나 있음
                errorCount++;
            }else if(opt1 != 0 && opt2 == 0){  // 오류 하나 있음
                errorCount++;
            }else{                  // 오류 두개 있음
                errorCount+=2;
            }
            

            // dataword 복원시키기
            char recoveredDataword[9] = {0,};
            strncpy(recoveredDataword, arr1, 4);
            strncpy(recoveredDataword+4, arr2, 4);


            // arr1, arr2 메모리 동적할당 해제
            free(arr1);
            free(arr2);

            // 8비트 바이너리 배열을 -> 실제 바이너리 코드로 바꿔줌
            unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char));
            binaryStringToBinary(recoveredDataword, data, 1);
            fwrite(data, 1, 1, output_file); // 바이너리로 복원된 dataword 넣기!

            free(data); // 동적할당 해제


        }else if(dataword_size == 8){

            int totalLen = (generator_size - 1) + padding_size + dataword_size;
            char* binaryString = (char*)malloc(sizeof(char) * totalLen + 1);

            int byteBoxSize = totalLen / 8;
            unsigned char* byteBox = (unsigned char*)malloc(sizeof(char)* byteBoxSize);
            
            // 한 덩어리 만큼 긁어오기
            read_count = fread(byteBox, sizeof(char), byteBoxSize, input_file);
            
            if(read_count != byteBoxSize){
                if(feof(input_file)){
                    break;
                }else{
                    printf("파일에서 데이터를 읽는데 실패3\n");
                }
            }
            
            codeWordCount++; // codeWord 하나 나왔으니 카운트 1 증가

            // 긁어온 바이너리 코드를 바이너리 문자열로 바꿔주기 -> binaryString에 담김
            binaryToBinaryString(byteBox, byteBoxSize, binaryString);

            // realCodeWord에는 padding을 제외한 바이너리 문자열이 담김
            int realCodeWordSize = totalLen - padding_size;
            char* realCodeWord = (char*)malloc(sizeof(char) * realCodeWordSize + 1);

            int j = padding_size;
            for(int i = 0; i < realCodeWordSize; i++, j++){
                realCodeWord[i] = binaryString[j];
            }

            realCodeWord[realCodeWordSize] = '\0';

            // 동적할당 해제할 것 해주기
            free(binaryString);
            free(byteBox);

            // 오류 체크하기
            int opt = crc_decoder(realCodeWord, generator);

            if(opt == 0){ // 오류 없는 경우

            }else if(opt == 1){ // 오류 있는 경우
                errorCount++;
            }

            // dataword 복원시키기
            char recoveredDataword[9] = {0};
            strncpy(recoveredDataword, realCodeWord, 8);

            free(realCodeWord); // 동적할당 해제
        
            unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char));
            binaryStringToBinary(recoveredDataword, data, 1);

            fwrite(data, sizeof(unsigned char), 1, output_file); // 바이너리로 복원된 dataword 넣기!

            free(data); // 동적할당 해제
        }

    }  

    // result file에 총 codeword 갯수와, 오류 뜬 개수 넣어주기;
    fprintf(result_file, "%d ", codeWordCount);
    fprintf(result_file, "%d", errorCount);

    // fclose 하기
    fclose(input_file);
    fclose(output_file);
    fclose(result_file);

    return 0;
}