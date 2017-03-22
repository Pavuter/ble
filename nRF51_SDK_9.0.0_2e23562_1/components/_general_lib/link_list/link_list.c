#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "DLib_Product_string.h"

#include "link_list.h"


typedef struct _double_link_list_t  
{  
    int data;  
    struct _double_link_list_t* prev;  
    struct _double_link_list_t* next;  
}double_link_list_t;


void assert(bool value){
  printf("");
}

double_link_list_t* create_double_link_node(int value)  
{  
    double_link_list_t* p_dll_node = NULL;  
    p_dll_node = (double_link_list_t*)malloc(sizeof(double_link_list_t));  
    assert(NULL != p_dll_node);  
  
    memset(p_dll_node, 0, sizeof(double_link_list_t));  
    p_dll_node->data = value;  
    return p_dll_node;  
}  


void delete_all_double_link_node(double_link_list_t** p_dll_node)  
{  
    double_link_list_t* p_node;  
    if(NULL == *p_dll_node)  
        return ;  
  
    p_node = *p_dll_node;  
    *p_dll_node = p_node->next;  
    free(p_node);  
    delete_all_double_link_node(p_dll_node);  
}  

double_link_list_t* find_data_in_double_link(const double_link_list_t* p_dll_node, int data)  
{  
    double_link_list_t* p_node = NULL;  
    if(NULL == p_dll_node)  
        return NULL;  
  
    p_node = (double_link_list_t*)p_dll_node;  
    while(NULL != p_node){  
        if(data == p_node->data)  
            return p_node;  
        p_node = p_node ->next;  
    }  
      
    return NULL;  
}  


bool insert_data_into_double_link(double_link_list_t** p_dll_node, int data)  
{  
    double_link_list_t* p_node;  
    double_link_list_t* p_index;  
  
    if(NULL == p_dll_node)  
        return false;  
  
    if(NULL == *p_dll_node){  
        p_node = create_double_link_node(data);  
        assert(NULL != p_node);  
        *p_dll_node = p_node;  
        (*p_dll_node)->prev = (*p_dll_node)->next = NULL;  
        return true;  
    }  
  
    if(NULL != find_data_in_double_link(*p_dll_node, data))  
        return false;  
  
    p_node = create_double_link_node(data);  
    assert(NULL != p_node);  
  
    p_index = *p_dll_node;  
    while(NULL != p_index->next)  
        p_index = p_index->next;  
  
    p_node->prev = p_index;  
    p_node->next = p_index->next;  
    p_index->next = p_node;  
    return true;  
}  

bool delete_data_from_double_link(double_link_list_t** p_dll_node, int data)  
{  
    double_link_list_t* p_node;  
    if(NULL == p_dll_node || NULL == *p_dll_node)  
        return false;  
  
    p_node = find_data_in_double_link(*p_dll_node, data);  
    if(NULL == p_node)  
        return false;  
  
    if(p_node == *p_dll_node){  
        if(NULL == (*p_dll_node)->next){  
            *p_dll_node = NULL;  
        }else{  
            *p_dll_node = p_node->next;  
            (*p_dll_node)->prev = NULL;  
        }  
  
    }else{  
        if(p_node->next)  
            p_node->next->prev = p_node->prev;  
        p_node->prev->next = p_node->next;  
    }  
  
    free(p_node);  
    return true;  
}  


int count_number_in_double_link(const double_link_list_t* p_dll_node)  
{  
    int count = 0;  
    double_link_list_t* p_node = (double_link_list_t*)p_dll_node;  
  
    while(NULL != p_node){  
        count ++;  
        p_node = p_node->next;  
    }  
    return count;  
}  


void print_double_link_node(const double_link_list_t* p_dll_node)  
{  
    double_link_list_t* p_node = (double_link_list_t*)p_dll_node;  
  
    while(NULL != p_node){  
        printf("%d\n", p_node->data);  
        p_node = p_node ->next;  
    }  
}  