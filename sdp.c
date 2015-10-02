#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "sdp.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif // _MSC_VER

static char* load_next_entry(char* p, char* key, char** value)
{
    char* end = NULL;

    if(NULL == p)
    {
        goto fail;
    }

    end = strstr(p, "\r\n");
    if(NULL == end)
    {
        end = strchr(p, '\n');
    }

    if(NULL != end)
    {
        while(*end == '\r' || *end == '\n')
        {
            *end++ = '\0';
        }
    }
    else
    {
        end = &p[strlen(p)];
    }

    if('\0' == p[0] || '=' != p[1])
    {
        goto fail;
    }

    *key = p[0];
    *value = &p[2];

    return end;

fail:
    *key = 0;
    *value = NULL;
    return NULL;
}

static char* split_values(char* p, char sep, char* fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    while(*p == sep)
    {
        p++;
    }

    char** s;
    char* tmp;
    int* i;
    long long int* l;
    time_t* t;

    while(*fmt)
    {
        switch(*fmt++)
        {
            case 's':
                s = va_arg(va, char**);
                *s = p;
                tmp = strchr(p, sep);
                if(tmp)
                {
                    p = tmp;
                    while(*p == sep)
                    {
                        *p++ = '\0';
                    }
                }
                else
                {
                    p = &p[strlen(p)];
                }
                break;
            case 'l':
                l = va_arg(va, long long int*);
                *l = strtoll(p, &tmp, 10);
                if(tmp == p)
                {
                    *p = 0;
                }
                else
                {
                    p = tmp;
                }
                break;
            case 'i':
                i = va_arg(va, int*);
                *i = strtol(p, &tmp, 10);
                if(tmp == p)
                {
                    *p = 0;
                }
                else
                {
                    p = tmp;
                }
                break;
            case 't':
                t = va_arg(va, time_t*);
                *t = strtol(p, &tmp, 10);
                if(tmp == p)
                {
                    *p = 0;
                }
                else
                {
                    p = tmp;
                    switch(*p)
                    {
                        case 'd':
                            *t *= 86400;
                            p++;
                            break;
                        case 'h':
                            *t *= 3600;
                            p++;
                            break;
                        case 'm':
                            *t *= 60;
                            p++;
                            break;
                    }
                }
                break;
        }
        while(*p == sep)
        {
            p++;
        }
    }
    va_end(va);
    return p;
}

#define GET_CONN_INFO(connf_ptr) do{if(key=='c'){sdp_connection* c=connf_ptr;split_values(value,' ',"sss",&c->nettype,&c->addrtype,&c->address);p=load_next_entry(p,&key,&value);}}while(0)

#define GET_BANDWIDTH_INFO(bw) do{int n;while(key=='b'){ADD_ENTRY(bw);n=bw##_count-1;split_values(value,':',"ss",&bw[n].bwtype,&bw[n].bandwidth);p=load_next_entry(p,&key,&value);}}while(0)

#define LOAD_FACULTATIVE_STR(k, field) do{if(key==k){field=value;p=load_next_entry(p,&key,&value);}}while(0)

#define LOAD_MULTIPLE_FACULTATIVE_STR(k, field) do{while(key==k){ADD_ENTRY(field);field[field##_count-1]=value;p=load_next_entry(p,&key,&value);}}while(0)

#define ADD_ENTRY(field) do{field##_count++;if(!field){field=calloc(1,sizeof(*field));}else{int n=field##_count;field=realloc(field,sizeof(*field)*n);memset(&field[n-1],0,sizeof(*field));}if(!(field))goto fail;}while(0)

sdp_payload* sdp_parse(const char* payload)
{
    sdp_payload* sdp = calloc(1, sizeof(sdp_payload));
    char* p;
    char key;
    char* value;
    char* end;

    if(NULL == sdp)
    {
        goto fail;
    }

    sdp->_payload = strdup(payload);
    p = sdp->_payload;
    if(NULL == p)
    {
        goto fail;
    }

    /* Protocol version (mandatory, only 0 supported) */
    p = load_next_entry(p, &key, &value);
    if('v' != key)
    {
        goto fail;
    }
    sdp->proto_version = strtol(value, &end, 10);
    if(0 != sdp->proto_version)
    {
        goto fail;
    }

    /* Origin field (mandatory) */
    p = load_next_entry(p, &key, &value);
    if('o' != key)
    {
        goto fail;
    }
    else
    {
        sdp_origin* o = &sdp->origin;
        split_values(value, ' ', "sllsss", &o->username, &o->sess_id, &o->sess_version, &o->nettype, &o->addrtype, &o->addr);
    }

    /* Session name field (mandatory) */
    p = load_next_entry(p, &key, &value);
    if('s' != key)
    {
        goto fail;
    }
    sdp->session_name = value;
    p = load_next_entry(p, &key, &value);

    /* Information field */
    // LOAD_FACULTATIVE_STR('i', sdp->information);
    do
    {
        if('i' == key)
        {
            sdp->information = value;
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* URI field */
    // LOAD_FACULTATIVE_STR('u', sdp->uri);
    do
    {
        if('u' == key)
        {
            sdp->uri = value;
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* Email addresses */
    // LOAD_MULTIPLE_FACULTATIVE_STR('e', sdp->emails);
    do
    {
        while('e' == key)
        {
            ADD_ENTRY(sdp->emails);
            sdp->emails[sdp->emails_count - 1] = value;
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* Phone numbers */
    // LOAD_MULTIPLE_FACULTATIVE_STR('p', sdp->phones);
    do
    {
        while('p' == key)
        {
            ADD_ENTRY(sdp->phones);
            sdp->phones[sdp->phones_count - 1] = value;
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* Connection information */
    // GET_CONN_INFO(&sdp->conn);
    do
    {
        if('c' == key)
        {
            sdp_connection* c = &sdp->conn;
            split_values(value, ' ', "sss", &c->nettype, &c->addrtype, &c->address);
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* Bandwidth fields */
    // GET_BANDWIDTH_INFO(sdp->bw);
    do
    {
        int n;
        while('b' == key)
        {
            ADD_ENTRY(sdp->bw);
            n = sdp->bw_count - 1;
            split_values(value, ':', "ss", &sdp->bw[n].bwtype, &sdp->bw[n].bandwidth);
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* Time fields (at least one mandatory) */
    do
    {
        sdp_time* tf;

        ADD_ENTRY(sdp->times);
        tf = &sdp->times[sdp->times_count - 1];
        split_values(value, ' ', "tt", &tf->start_time, &tf->stop_time);
        p = load_next_entry(p, &key, &value);

        while('r' == key)
        {
            sdp_repeat* rf;

            ADD_ENTRY(tf->repeat);
            rf = &tf->repeat[tf->repeat_count - 1];
            value = split_values(value, ' ', "tt", &rf->interval, &rf->duration);
            while('\0' != *value)
            {
                int n = rf->offsets_count;
                ADD_ENTRY(rf->offsets);
                value = split_values(value, ' ', "t", &rf->offsets[n]);
            }
            p = load_next_entry(p, &key, &value);
        }
    } while('t' == key);

    /* Zone adjustments */
    if('z' == key)
    {
        while('\0' != *value)
        {
            int n = sdp->zone_adjustments_count;
            sdp_zone_adjustments* za;

            ADD_ENTRY(sdp->zone_adjustments);
            za = &sdp->zone_adjustments[n];
            value = split_values(value, ' ', "tt", &za->adjust, &za->offset);
        }
        p = load_next_entry(p, &key, &value);
    }

    /* Encryption key */
    // LOAD_FACULTATIVE_STR('k', sdp->encrypt_key);
    do
    {
        if('k' == key)
        {
            sdp->encrypt_key = value;
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* Media attributes */
    // LOAD_MULTIPLE_FACULTATIVE_STR('a', sdp->attributes);
    do
    {
        while('a' == key)
        {
            ADD_ENTRY(sdp->attributes);
            sdp->attributes[sdp->attributes_count - 1] = value;
            p = load_next_entry(p, &key, &value);
        }
    } while(0);

    /* Media descriptions */
    while('m' == key)
    {
        sdp_media* md;

        ADD_ENTRY(sdp->medias);
        md = &sdp->medias[sdp->medias_count - 1];

        value = split_values(value, ' ', "s", &md->info.type);
        md->info.port = strtol(value, &value, 10);
        md->info.port_n = *value == '/' ? strtol(value + 1, &value, 10) : 0;
        value = split_values(value, ' ', "s", &md->info.proto);
        while('\0' != *value)
        {
            ADD_ENTRY(md->info.fmt);
            value = split_values(value, ' ', "i", &md->info.fmt[md->info.fmt_count - 1]);
        }
        p = load_next_entry(p, &key, &value);

        // LOAD_FACULTATIVE_STR('i', md->title);
        do
        {
            if('i' == key)
            {
                md->title = value;
                p = load_next_entry(p, &key, &value);
            }
        } while(0);
        // GET_CONN_INFO(&md->conn);
        do
        {
            if('c' == key)
            {
                sdp_connection* c = &md->conn;
                split_values(value, ' ', "sss", &c->nettype, &c->addrtype, &c->address);
                p = load_next_entry(p, &key, &value);
            }
        } while(0);
        // GET_BANDWIDTH_INFO(md->bw);
        do
        {
            int n; while('b' == key)
            {
                ADD_ENTRY(md->bw);
                n = md->bw_count - 1;
                split_values(value, ':', "ss", &md->bw[n].bwtype, &md->bw[n].bandwidth);
                p = load_next_entry(p, &key, &value);
            }
        } while(0);
        // LOAD_FACULTATIVE_STR('k', md->encrypt_key);
        do
        {
            if('k' == key)
            {
                md->encrypt_key = value;
                p = load_next_entry(p, &key, &value);
            }
        } while(0);
        // LOAD_MULTIPLE_FACULTATIVE_STR('a', md->attributes);
        do
        {
            while('a' == key)
            {
                ADD_ENTRY(md->attributes);
                md->attributes[md->attributes_count - 1] = value;
                p = load_next_entry(p, &key, &value);
            }
        } while(0);
    }

    return sdp;

fail:
    sdp_destroy(sdp);
    return NULL;
}

void sdp_destroy(sdp_payload* sdp)
{
    size_t i, j;

    if(NULL != sdp)
    {
        free(sdp->_payload);
        free(sdp->emails);
        free(sdp->phones);
        free(sdp->bw);
        for(i = 0; i < sdp->times_count; i++)
        {
            for(j = 0; j < sdp->times[i].repeat_count; j++)
            {
                free(sdp->times[i].repeat[j].offsets);
            }
            free(sdp->times[i].repeat);
        }
        free(sdp->times);
        free(sdp->zone_adjustments);
        free(sdp->attributes);
        for(i = 0; i < sdp->medias_count; i++)
        {
            free(sdp->medias[i].info.fmt);
            free(sdp->medias[i].bw);
            free(sdp->medias[i].attributes);
        }
        free(sdp->medias);
    }
    free(sdp);
}

char* sdp_get_attr(char** attr, size_t nattr, char* key)
{
    size_t i;
    size_t klen = strlen(key);

    for(i = 0; i < nattr; i++)
    {
        if(0 == strncmp(attr[i], key, klen) && ':' == attr[i][klen])
        {
            return &attr[i][klen + 1];
        }
    }
    return NULL;
}

int sdp_has_flag_attr(char** attr, size_t nattr, char* flag)
{
    size_t i;

    for(i = 0; i < nattr; i++)
    {
        if(0 == strcmp(attr[i], flag))
        {
            return 1;
        }
    }
    return 0;
}

void sdp_dump(sdp_payload* sdp)
{
    size_t i;
    size_t j;
    size_t k;

    if(NULL == sdp)
    {
        printf("invalid SDP\n");
        return;
    }

    printf("v=%d\n", sdp->proto_version);
    printf("o=%s %lld %lld %s %s %s\n",
           sdp->origin.username,
           sdp->origin.sess_id,
           sdp->origin.sess_version,
           sdp->origin.nettype,
           sdp->origin.addrtype,
           sdp->origin.addr);
    printf("s=%s\n", sdp->session_name);

    if(sdp->information)
    {
        printf("i=%s\n", sdp->information);
    }
    if(sdp->uri)
    {
        printf("u=%s\n", sdp->uri);
    }

    for(i = 0; i < sdp->emails_count; i++)
    {
        printf("e=%s\n", sdp->emails[i]);
    }
    for(i = 0; i < sdp->phones_count; i++)
    {
        printf("p=%s\n", sdp->phones[i]);
    }

    if(sdp->conn.nettype && sdp->conn.addrtype && sdp->conn.address)
    {
        printf("c=%s %s %s\n", sdp->conn.nettype, sdp->conn.addrtype, sdp->conn.address);
    }

    for(i = 0; i < sdp->bw_count; i++)
    {
        printf("b=%s:%s\n", sdp->bw[i].bwtype, sdp->bw[i].bandwidth);
    }

    for(i = 0; i < sdp->times_count; i++)
    {
        sdp_time* t = &sdp->times[i];
        printf("t=%lld %lld\n", t->start_time, t->stop_time);
        for(j = 0; j < t->repeat_count; j++)
        {
            sdp_repeat* r = &t->repeat[j];
            printf("r=%lld %lld", r->interval, r->duration);
            for(k = 0; k < r->offsets_count; k++)
            {
                printf(" %lld", r->offsets[k]);
            }
            printf("\n");
        }
    }

    if(sdp->zone_adjustments_count)
    {
        printf("z=");
        for(i = 0; i < sdp->zone_adjustments_count; i++)
        {
            printf("%lld %lld ",
                   sdp->zone_adjustments[i].adjust,
                   sdp->zone_adjustments[i].offset);
        }
        printf("\n");
    }

    if(sdp->encrypt_key)
    {
        printf("k=%s\n", sdp->encrypt_key);
    }

    for(i = 0; i < sdp->attributes_count; i++)
    {
        printf("a=%s\n", sdp->attributes[i]);
    }

    for(i = 0; i < sdp->medias_count; i++)
    {
        sdp_media* m = &sdp->medias[i];
        sdp_info* info = &m->info;

        printf("m=%s %d", info->type, info->port);
        if(info->port_n)
        {
            printf("/%d", info->port_n);
        }
        printf(" %s", info->proto);
        for(j = 0; j < info->fmt_count; j++)
        {
            printf(" %d", info->fmt[j]);
        }
        printf("\n");

        if(m->title)
        {
            printf("i=%s\n", m->title);
        }
        if(m->conn.nettype && m->conn.addrtype && m->conn.address)
        {
            printf("c=%s %s %s\n",
                   m->conn.nettype,
                   m->conn.addrtype,
                   m->conn.address);
        }
        for(j = 0; j < m->bw_count; j++)
        {
            printf("b=%s:%s\n", m->bw[j].bwtype, m->bw[j].bandwidth);
        }
        if(m->encrypt_key)
        {
            printf("k=%s\n", m->encrypt_key);
        }
        for(j = 0; j < m->attributes_count; j++)
        {
            printf("a=%s\n", m->attributes[j]);
        }
    }
}
