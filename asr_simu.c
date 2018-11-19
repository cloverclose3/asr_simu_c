#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "curl/curl.h"

#define FALSE 0
#define TRUE 1
#define MAX_BUF     1024*1024 

const char *token;
char wr_buf[MAX_BUF+1];
int  wr_index;

extern char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length);
extern unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length);
extern void build_decoding_table();
extern void base64_cleanup();


/* 
* Write data callback function (called within the context of 
* curl_easy_perform. 
*/ 
size_t write_data( void *buffer, size_t size, size_t nmemb, void *userp ) 
{ 
    int segsize = size * nmemb;

    if ( wr_index + segsize > MAX_BUF ) {
        return 0;
    }

    /* Copy the data from the curl buffer into our buffer */
    memcpy( (void *)&wr_buf[wr_index], buffer, (size_t)segsize );

    /* Update the write index */
    wr_index += segsize;

    /* Null terminate the buffer */
    wr_buf[wr_index] = 0;

    /* Return the number of bytes received, indicating to curl that all is okay */
    return segsize;
} 

const char * getAccessToken_needfree()
{
    CURL * curl = curl_easy_init();
    struct curl_slist * slist = NULL;
    int ret;
    const char *ret_buf, *token_str;
    cJSON *root = NULL, *token_item;
    time_t now = time(NULL);

    // 这里需要填写你申请到的 AK SK
    curl_easy_setopt(curl, CURLOPT_URL, "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=你的AK&client_secret=你的SK");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    ret = curl_easy_perform(curl);
    if ( ret == 0 ) {
        printf( "%s\n", wr_buf );
        // parse, get "access_token"
        root = cJSON_Parse(wr_buf);
        if (root) {
            if ((token_item = cJSON_GetObjectItem(root, "access_token"))) {
                token_str = cJSON_GetStringValue(token_item);
                printf("token_str: %s\n", token_str);
                ret_buf = strndup(token_str, strlen(token_str) + 1);
                cJSON_Delete(root);
                wr_index = 0;
                curl_easy_cleanup(curl);
                curl_slist_free_all(slist);
                return ret_buf;
            }
        }
    }

    printf("post error1\n");
    wr_index = 0;
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    return NULL;
}

const char* request_asr(const char *asr, const char *json)
{
    int ret;
    struct curl_slist * slist = NULL;
    CURL * curl = curl_easy_init();
    cJSON *root = NULL, *res_item, *str_item;
    const char *res_str, *ret_buf;

    slist = curl_slist_append(slist, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, asr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(json) + 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    ret = curl_easy_perform(curl);
    if ( ret == 0 ) {
        printf( "recognize results: %s\n", wr_buf );
        
        root = cJSON_Parse(wr_buf);
        if (root) {
            if ((res_item = cJSON_GetObjectItem(root, "result"))) {
                if (cJSON_IsArray(res_item)) {
                    int sz;
                    
                    sz = cJSON_GetArraySize(res_item);
                    printf("array sz: %d\n", sz);
                    if (sz > 0) {
                        str_item = cJSON_GetArrayItem(res_item, 0);
                        if (cJSON_IsString(str_item)) {
                            res_str = cJSON_GetStringValue(str_item);
                            printf("res_str: %s\n", res_str);
                            ret_buf = strndup(res_str, strlen(res_str) + 1);
                            
                            cJSON_Delete(root);
                            wr_index = 0;
                            curl_easy_cleanup(curl);
                            curl_slist_free_all(slist);
                            return ret_buf;
                        }
                    }
                }
            }
        } else {
            printf("json parse failed %d\n", __LINE__);
        }

    }
    
    
    printf("post error2\n");
    wr_index = 0;
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    return NULL;
}

int convert_stereo_to_mono(const char *wav_file, const char *pcm_file)
{
    
    char wav_header[44];
    char buf[16] = {0};
    static int leftFlag = FALSE;
    int size = 0;

    
    FILE *fp = fopen(wav_file, "rb+");
    FILE *fp_out = fopen(pcm_file, "wb+");

    // first skip 44 bytes wav header
    fread(wav_header, 1, 44, fp);
    while(!feof(fp))
    {
        size = fread(buf, 1, 2, fp); // 此处是读取16bit（一个声道），一个字节8位，所以count为2

        if( (size>0) && (leftFlag == FALSE) )
        {
            // 左声道数据
            leftFlag = TRUE;
            fwrite(buf, 1, size, fp_out);
        }
        else if( (size>0) && (leftFlag == TRUE) )
        {
            // 右声道数据
            leftFlag = FALSE;
            //fwrite(buf, 1, size, fp_out);
        }
    }

    fclose(fp);
    fclose(fp_out);
    
    return 0;
}

char rfc3986[256] = {0};
char html5[256] = {0};

void url_encoder_rfc_tables_init(){

    int i;

    for (i = 0; i < 256; i++){

        rfc3986[i] = isalnum( i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
        html5[i] = isalnum( i) || i == '*' || i == '-' || i == '.' || i == '_' ? i : (i == ' ') ? '+' : 0;
    }
}

char *url_encode( char *table, unsigned char *s, char *enc){

    for (; *s; s++){

        if (table[*s]) sprintf( enc, "%c", table[*s]);
        else sprintf( enc, "%%%02X", *s);
        while (*++enc);
    }

    return( enc);
}


int request_tts(const char *chinese, const char *mp3_file)
{
    int ret;
    struct curl_slist * slist = NULL;
    CURL * curl = curl_easy_init();
    cJSON *root = NULL;
    char post_data[512];
    char url_encoded[1024];
    
    strcat(post_data, "tex=");
    //strcat(post_data, chinese);
    url_encode( html5, chinese, url_encoded);
    strcat(post_data, url_encoded);
    strcat(post_data, "&lan=zh");
    // 这里需要填写你的 AK
    strcat(post_data, "&cuid=你的AK");
    strcat(post_data, "&ctp=1");
    strcat(post_data, "&tok=");
    strcat(post_data, token);

    curl_easy_setopt(curl, CURLOPT_URL, "http://tsn.baidu.com/text2audio");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data) + 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    ret = curl_easy_perform(curl);
    if ( ret == 0 ) {
        // wr_buf: contain the mp3 data or error message
        if ((root = cJSON_Parse(wr_buf))) {
            printf("post error: %s\n", wr_buf);
            cJSON_Delete(root);
            goto ERR_OUT;
        }
        FILE *fp_out = fopen(mp3_file, "wb+");
        if (wr_index != fwrite(wr_buf, 1, wr_index, fp_out)) {
            printf("write to mp3 file error!\n");
            fclose(fp_out);
            goto ERR_OUT;
        }

        printf("write to mp3 file success!\n");
        fclose(fp_out);
        wr_index = 0;
        curl_easy_cleanup(curl);
        curl_slist_free_all(slist);
        return 0;
    }
    
    printf("post error3\n");
ERR_OUT:
    wr_index = 0;
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    return -1;
}


int main(int argc, char **argv)
{
    char pcm_file[32], mp3_file[32], *p;
    size_t out_len, pcm_len;
    const char *out_data, *pcm_data, *jstr;
    cJSON *root = NULL;

    if (argc != 2) {
        printf("Usage: ./asr_simu input.wav\n");
        return -1;
    }
    
    /* The format of xxx.wav file comes from 'arecord' is "2 channel 16k 16bit".
     * Convert it to "1 channel 16k 16bit" format.
     */
    strncpy(pcm_file, argv[1], sizeof(pcm_file));
    pcm_file[sizeof(pcm_file) - 1] = '\0';
    strncpy(mp3_file, argv[1], sizeof(mp3_file));
    mp3_file[sizeof(mp3_file) - 1] = '\0';
    if ((p = strstr(pcm_file, ".wav"))) {
        strcpy(p, ".pcm");
        printf("pcm_file: %s\n", pcm_file);
        p = strstr(mp3_file, ".wav");
        strcpy(p, ".mp3");
        printf("mp3_file: %s\n", mp3_file);
    } else {
        printf("input file is not a .wav file\n");
        return -1;
    }
    
    if (convert_stereo_to_mono(argv[1], pcm_file)) {
        printf("convert_stereo_to_mono() error\n");
        return -1;
    }
    
    FILE *fp = fopen(pcm_file, "rb");

    // get file size from file info
    struct stat finfo;
    fstat(fileno(fp), &finfo);
    pcm_len = finfo.st_size;

    pcm_data = malloc(sizeof(unsigned char)*pcm_len);
    if( fread(pcm_data, 1, pcm_len, fp) != pcm_len) {
        printf("fread() error");
        goto ERR_FREE_DATA;
    }

    build_decoding_table();
    out_data = base64_encode(pcm_data, pcm_len, &out_len);
    //printf("out_data:\n%s\n", out_data);
#if 0
    token = getAccessToken_needfree();
    if (!token) {
        printf("getAccessToken() failed\n");
        goto ERR_CLEAN_TALBE;
    }
#else
    token = "24.3ec82a845ee0957a2d906fa55f0d529f.2592000.1545141536.282335-14813770";
#endif
    

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "speech", out_data);
    cJSON_AddStringToObject(root, "format", "pcm");
    cJSON_AddNumberToObject(root, "rate", 16000);
    cJSON_AddStringToObject(root, "channel", "1");
    cJSON_AddStringToObject(root, "token", token);
    cJSON_AddStringToObject(root, "cuid", "你的AK"); //这里需要填写你的 ak
    cJSON_AddNumberToObject(root, "len", pcm_len);

    jstr = cJSON_Print(root);
    
    const char *_asr = "https://vop.baidu.com/server_api";
    const char *chinese;
    chinese = request_asr(_asr, jstr);
    
    if (!chinese) {
        printf("recognize failed\n");
        goto ERR_FREE_JSTR;
    }
    
    printf("chinese: %s\n", chinese);
    request_tts(chinese, mp3_file);
    
    
    
    
    
    free(chinese);
    if (token) {
        free(token);
    }
    
    return 0;
ERR_FREE_JSTR:
    free(jstr);
ERR_DEL_ROOT:
    cJSON_Delete(root);
ERR_CLEAN_TALBE:
    base64_cleanup();
ERR_FREE_DATA:
    free(pcm_data);
ERR_CLOSE_FP:
    fclose(fp);
ERR_OUT:
    return -1;
}
