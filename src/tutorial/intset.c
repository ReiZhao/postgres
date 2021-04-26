#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "math.h"

#include "postgres.h"

#include "fmgr.h"
#define hashBound 1021

PG_MODULE_MAGIC;


typedef struct ListNode{
    int value;
    struct ListNode * next;
}ListNode;

typedef struct Set{
    int capacity;
    struct ListNode ** list;
    int * members;
}Set;


typedef struct IntSet{
    int32 length;
    int capacity;
    int members[FLEXIBLE_ARRAY_MEMBER];
}intSet;

int hashIndex(int value);
void setInsert(Set *Set, char * elements, const char * delim);
void hashInsert(Set *Set, int value, int index);
Set * SetInitiate(char * elements);
void SetFree(Set *Set);
int cmpfunc (const void * x, const void * y);
void setOutput(Set *Set);
void inputCheck(char * str);
int binarySearch(int * arr, int left, int right, int x);
int * unionSet(int * A, int * B, int * count, int ACapacity, int BCapacity);
int * difference(int * A, int * B, int * count, int ACapacity, int BCapacity);


int hashIndex(int value){
    return value % hashBound;
}

int cmpfunc (const void * x, const void * y) {
    return (* (int*) x - * (int*) y) ;
}


void inputCheck(char * input){
	char * str;
    int commaFlag;

    str = input;
    commaFlag = 0;


    if(*str == '{'){
        str ++;
    }

    else{
        str += strspn(str, " ");
        if(*str != '{'){
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("invalid input syntax for type %s (invalid header) : \"%s\"",
                        "intset", str)));
        }
        str ++;
    }

     while(*str != '\0'){
        if('0' <= *str && *str <= '9'){
            if(commaFlag) commaFlag = 0;
        }
        else if(*str == ',' && (!commaFlag)){
            commaFlag = 1;
        }
        else if(*str == ' '){
        }
        else if(*str == '}'){
            if(*(str + 1 + strspn(str + 1, " ")) != '\0'){
                ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid input syntax for type %s (invalid ending) : \"%s\"",
                        "intset", str)));
            }
        }
        else{
            ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid input syntax for type %s (invalid character) : \"%s\"",
                        "intset", str)));
        }
        str ++;
    }
}

void setInsert(Set *set, char * elements, const char * delim){

	char * input;
	long ret;
	int hash_index;

    input = elements;
    input += strspn(input, delim);
    ret = 0;
    hash_index = 0;

    // find a target char
    while(*input != '\0'){
        ret = strtol(input, &input, 10);
        input += strspn(input, delim);
       
        hash_index = hashIndex((int) ret);
        hashInsert(set, (int)ret, hash_index);
        
    }
}

void hashInsert(Set *Set, int value, int index){
    
    ListNode * root;
    ListNode * list_node;
    ListNode * listHeader;

    root = NULL;
    list_node = NULL;
    listHeader = NULL;

    root = Set->list[index];

    // a root without any elements storing here
    if(root->next == NULL){
        list_node = malloc(sizeof(ListNode));
        list_node->value = value;
        list_node->next = NULL;
        root->next = list_node;
        root->value ++;
        Set->capacity ++;
        return;
    }
    // a root has stored some elements
    // check if the element has been in this list
    listHeader = root->next;
    while(listHeader!= NULL){
        if(listHeader->value == value) return;
        listHeader = listHeader->next;
    }
    // element not in this list yet
    list_node = malloc(sizeof(ListNode));
    list_node->value = value;
    list_node->next = root->next;
    root->next = list_node;
    root->value ++;
    Set->capacity ++;
}

Set * SetInitiate(char * elements){
    //initiate struct intSet
    Set *set;
    int i;

    set = malloc(sizeof(intSet));
    set->capacity = 0;
    set->list = malloc(sizeof(ListNode *) * hashBound);
    
    for(i =0 ; i < hashBound; i++){
        set->list[i] = malloc(sizeof(ListNode));
        //set->list[i]->memberNums = 0;
        set->list[i]->value = 0;
        set->list[i]->next = NULL;
    }
    setInsert(set, elements, "{, }");
    set->members = calloc(set->capacity, set->capacity * sizeof(int));

    //ereport(WARNING, (errmsg("HashSet initiate finished\n")));

    return set;
}

void SetFree(Set *Set){
    ListNode * p = NULL;
    ListNode * q = NULL;
    int i;

    for(i=0; i < hashBound; i++){
        if(Set->list[i] == NULL) continue;
        p = Set->list[i];
        while(p->next != NULL){
            q = p->next;
            free(p);
            p = q;
        }
        free(p);
    }
    p = NULL; q = NULL;
    free(Set->list);
    Set->list = NULL;

    if(Set->members != NULL)    free(Set->members);
    Set->members = NULL;

    free(Set);
    Set = NULL;

    //ereport(WARNING, (errmsg("HashSet Free finished\n")));
}

int binarySearch(int * arr, int left, int right, int x) {
	int mid;
  	if(right >= left)
   {
        mid = left + (right - left)/2;
        if (arr[mid] == x)  
            return mid;

        if (arr[mid] > x) 
            return binarySearch(arr, left, mid-1, x);

        return binarySearch(arr, mid+1, right, x);
   }
   return -1;
}

void setOutput(Set *Set){
    int setCapacity;
    int index;
    int i;
    ListNode * p;

    setCapacity = Set->capacity;
    index = 0;

    if(!setCapacity) {
        return;
    }
    // quick sort
    
    for(i = 0; i < hashBound; i++){
        p = Set->list[i]->next;
        while(p != NULL){
            Set->members[index] = p->value;
            p = p->next;
            index ++;
        }
    }
    qsort(Set->members, setCapacity, sizeof(int), cmpfunc);
    //ereport(WARNING, (errmsg("HashSet list output finished\n")));
}

int * unionSet(int * A, int * B, int * count, int ACapacity, int BCapacity){
	int *unionset;
    int maxValue;
    int i;
    int searchStopIndex;
    int searchResult;


	unionset = calloc(ACapacity + BCapacity, sizeof(int));
    // # 1 copy A to union
    memcpy(unionset, A, sizeof(int) * ACapacity);

   	maxValue = A[ACapacity - 1] > B[BCapacity - 1]? A[ACapacity - 1]: B[BCapacity - 1];
    
    for(i = ACapacity; i < ACapacity + BCapacity; i++){
        unionset[i] = maxValue;
    }
    // # 2 Add new elements in B into union
    searchStopIndex = 0;
    searchResult = 0;
    (*count) = ACapacity;

    for(i = 0; i < BCapacity; i++){
        searchResult = binarySearch(A, searchStopIndex, ACapacity - 1, B[i]);
        if(searchResult == -1){
            unionset[*count] = B[i];
            (*count) ++;
            continue;
        }
        searchStopIndex = searchStopIndex > searchResult? searchStopIndex: searchResult;
    }
    return unionset;

}

// diffset[:count] is the result
int * difference(int * A, int * B, int * count, int ACapacity, int BCapacity){

	int * diffset;
	int searchStopIndex;
	int searchResult;
	int deleteLabel;
	int i;


    diffset = calloc(ACapacity, sizeof(int));
    searchStopIndex = 0;
    searchResult = 0;
    (*count) = ACapacity;
    deleteLabel = A[ACapacity - 1] + 1;
    // # 1 copy A to union
    memcpy(diffset, A, sizeof(int) * ACapacity);
    // # 2 Add new elements in B into union


    for(i = 0; i < BCapacity; i++){
        searchResult = binarySearch(A, searchStopIndex, ACapacity- 1, B[i]);
        if(searchResult > -1){
            diffset[searchResult] = deleteLabel;
            (*count) --;
            continue;
        }
        searchStopIndex = searchStopIndex > searchResult? searchStopIndex: searchResult;
    }
    qsort(diffset, ACapacity, sizeof(int), cmpfunc);
    return diffset;

}


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_in);

Datum
intset_in(PG_FUNCTION_ARGS)
{
	char * str;
	Set * internalSet;

	intSet * int_set;

    str = (char *) PG_GETARG_CSTRING(0);

    inputCheck(str);
    //initiate the internal structure hashSet
    internalSet = SetInitiate(str);

    if(!internalSet->capacity){
        int_set = (intSet *) palloc(VARHDRSZ + sizeof(int));
        SET_VARSIZE(int_set, VARHDRSZ + sizeof(int));
        int_set->capacity = internalSet->capacity;
        SetFree(internalSet);
        PG_RETURN_POINTER(int_set);

    }

    setOutput(internalSet);
    int_set = (intSet *) palloc(VARHDRSZ + sizeof(int) * (internalSet->capacity + 1));
    SET_VARSIZE(int_set, VARHDRSZ + sizeof(int) * (internalSet->capacity + 1));
    int_set->capacity = internalSet->capacity;
    memcpy(int_set->members, internalSet->members, sizeof(int) * (internalSet->capacity));
    SetFree(internalSet);

    PG_RETURN_POINTER(int_set);
}



PG_FUNCTION_INFO_V1(intset_out);


Datum
intset_out(PG_FUNCTION_ARGS)
{
	intSet * Set;
	char * result;
	int m;

    Set = (intSet *) PG_GETARG_POINTER(0);
    result = "{";
    
    if(Set->capacity == 0){
        result = "{}";
    }
    else{
        for(m = 0; m < Set->capacity; m++){
            if(m == Set->capacity - 1)
                result = psprintf("%s%d}", result, Set->members[m]);
            else
                result = psprintf("%s%d,", result, Set->members[m]);
        }
    }
    PG_RETURN_CSTRING(result);
}



/*****************************************************************************
 * New Operators i ? S
 *
 * check in
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_checkin);

Datum
intset_checkin(PG_FUNCTION_ARGS)
{
	int x;
	intSet * set;

    x = PG_GETARG_INT32(0);
    set = (intSet *) PG_GETARG_POINTER(1);
    PG_RETURN_BOOL(binarySearch(set->members, 0, set->capacity - 1, x) > -1);
}

/*****************************************************************************
 * New Operators # S
 *
 * cadinality of set S
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_cap);

Datum
intset_cap(PG_FUNCTION_ARGS)
{
	intSet *set;

    set = (intSet *) PG_GETARG_POINTER(0);
    PG_RETURN_INT32(set->capacity);
}

/*****************************************************************************
 * New Operators A @< B
 *
 * Does B is a superset of A
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_superclass);

Datum
intset_superclass(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
	int i;
	int searchStopIndex = 0;

    A = (intSet *) PG_GETARG_POINTER(0);
    B = (intSet *) PG_GETARG_POINTER(1);
    // A has more elements, B is not one superSet of A
    if(A->capacity > B->capacity) PG_RETURN_BOOL(0);

    

    for(i = 0; i < A->capacity; i++){
        searchStopIndex = binarySearch(B->members, searchStopIndex, B->capacity - 1, A->members[i]);
        if(searchStopIndex == -1)
            PG_RETURN_BOOL(0);
    }
    PG_RETURN_BOOL(1);
}



/*****************************************************************************
 * New Operators A >@ B
 *
 * Does A is a superset of B
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_subclass);

Datum
intset_subclass(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
	int i;
	int searchStopIndex = 0;

    B = (intSet *) PG_GETARG_POINTER(0);
    A = (intSet *) PG_GETARG_POINTER(1);

    if(A->capacity > B->capacity) PG_RETURN_BOOL(0);

    for(i = 0; i < A->capacity; i++){
        searchStopIndex = binarySearch(B->members, searchStopIndex, B->capacity - 1, A->members[i]);
        if(searchStopIndex == -1)
            PG_RETURN_BOOL(0);
    }
    PG_RETURN_BOOL(1);
}

/*****************************************************************************
 * New Operators A = B
 *
 * Does A is equal to B
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_equal);

Datum
intset_equal(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
	int i;

    A = (intSet *) PG_GETARG_POINTER(0);
    B = (intSet *) PG_GETARG_POINTER(1);

    if(A->capacity != B->capacity) PG_RETURN_BOOL(0);

    for(i = 0; i < A->capacity; i++){
        if(A->members[i] != B->members[i]) PG_RETURN_BOOL(0);
    }
    PG_RETURN_BOOL(1);
}

/*****************************************************************************
 * New Operators A <> B
 *
 * Does A is not equal to B
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_notequal);

Datum
intset_notequal(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
	int i;

    A = (intSet *) PG_GETARG_POINTER(0);
    B = (intSet *) PG_GETARG_POINTER(1);

    if(A->capacity != B->capacity) PG_RETURN_BOOL(1);

    for(i = 0; i < A->capacity; i++){
        if(A->members[i] != B->members[i])  PG_RETURN_BOOL(1);
    }

    PG_RETURN_BOOL(0);
}


/*****************************************************************************
 * New Operators A || B

 *
 * return the union of A and B
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_union);

Datum
intset_union(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
	int unionSetMembers;
	int * unionResult;
	intSet * union_set;

    A = (intSet *) PG_GETARG_POINTER(0);
    B = (intSet *) PG_GETARG_POINTER(1);

    unionSetMembers = A->capacity;

    unionResult = unionSet(A->members, B->members, &unionSetMembers, A->capacity, B->capacity);

    union_set = (intSet *) palloc(VARHDRSZ + sizeof(int) * (unionSetMembers + 1));
    SET_VARSIZE(union_set, VARHDRSZ + sizeof(int) * (unionSetMembers + 1));
    union_set->capacity = unionSetMembers;
    // copy when #elements > 0
    if(unionSetMembers){
        qsort(unionResult, unionSetMembers, sizeof(int), cmpfunc);
        memcpy(union_set->members, unionResult, sizeof(int) * unionSetMembers);
    }


    free(unionResult);
    PG_RETURN_POINTER(union_set);
}



/*****************************************************************************
 * New Operators A - B

 *
 * return the difference of A and B
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_diff);

Datum
intset_diff(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
	int diffMembers;
	int * diffResult;
	intSet * diff_set;


    A = (intSet *) PG_GETARG_POINTER(0);
    B = (intSet *) PG_GETARG_POINTER(1);

    diffMembers = A->capacity;

    diffResult = (int *) difference(A->members, B->members, &diffMembers, A->capacity, B->capacity);


    diff_set = palloc(VARHDRSZ + sizeof(int) * (diffMembers + 1));
    SET_VARSIZE(diff_set, VARHDRSZ + sizeof(int) * (diffMembers + 1));
    diff_set->capacity = diffMembers;
    // copy when #elements > 0
    if(diffMembers)
        qsort(diffResult, diffMembers, sizeof(int), cmpfunc);
        memcpy(diff_set->members, diffResult, sizeof(int) * diffMembers);

    free(diffResult);

    PG_RETURN_POINTER(diff_set);
}


/*****************************************************************************
 * New Operators A !! B = A - B \u B - A

 *
 * return the disjunction of A and B
*****************************************************************************/

PG_FUNCTION_INFO_V1(intset_disjunc);

Datum
intset_disjunc(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
	int diff_ABCount;
	int diff_BACount;

	int * diffAB;
	int * diffBA;
	intSet * intset_disjunc;

	int unionCount;
    int * unionAB;
    int * unionBA;
	int * unionResult;

	unionCount = 0;
	unionAB = NULL;
	unionBA = NULL;
	unionResult = NULL;

    A = (intSet *) PG_GETARG_POINTER(0);
    B = (intSet *) PG_GETARG_POINTER(1);

    diff_ABCount = A->capacity;
    diff_BACount = B->capacity;

    diffAB = difference(A->members, B->members, &diff_ABCount, A->capacity, B->capacity);
    diffBA = difference(B->members, A->members, &diff_BACount, B->capacity, A->capacity);


    if((!diff_ABCount) && (!diff_BACount)){
        intset_disjunc = palloc(VARHDRSZ + sizeof(int));
        SET_VARSIZE(intset_disjunc, VARHDRSZ + sizeof(int));
        intset_disjunc->capacity = 0;
        
    }

    else if(!diff_ABCount && diff_BACount){
        intset_disjunc = palloc(VARHDRSZ + sizeof(int) * (1 + diff_BACount));
        SET_VARSIZE(intset_disjunc, VARHDRSZ + sizeof(int) * (1 + diff_BACount));
        intset_disjunc->capacity = diff_BACount;
        qsort(diffBA, diff_BACount, sizeof(int), cmpfunc);
        memcpy(intset_disjunc->members, diffBA, sizeof(int) * diff_BACount);
    }

    else if (!diff_BACount && diff_ABCount){
        intset_disjunc = palloc(VARHDRSZ + sizeof(int) * (1 + diff_ABCount));
        SET_VARSIZE(intset_disjunc, VARHDRSZ + sizeof(int) * (1 + diff_ABCount));
        intset_disjunc->capacity = diff_ABCount;
        qsort(diffAB, diff_ABCount, sizeof(int), cmpfunc);
        memcpy(intset_disjunc->members, diffAB, sizeof(int) * diff_ABCount);
    }

    else{
        unionCount = diff_ABCount;
        unionAB = calloc(diff_ABCount, sizeof(int));
        unionBA = calloc(diff_BACount, sizeof(int));
        memcpy(unionAB, diffAB, sizeof(int) * diff_ABCount);
        memcpy(unionBA, diffBA, sizeof(int) * diff_BACount);

        unionResult = unionSet(unionAB, unionBA, &unionCount, diff_ABCount, diff_BACount);

        free(unionAB);
        free(unionBA);
        
        intset_disjunc = palloc(VARHDRSZ + sizeof(int) * (1 + unionCount));
        SET_VARSIZE(intset_disjunc, VARHDRSZ + sizeof(int) * (1 + unionCount));
        intset_disjunc->capacity = unionCount;
        qsort(unionResult, unionCount, sizeof(int), cmpfunc);
        memcpy(intset_disjunc->members, unionResult, sizeof(int) * unionCount);

        free(unionResult); 
    }


    free(diffAB);
    free(diffBA);

    PG_RETURN_POINTER(intset_disjunc);
}


/*****************************************************************************
 * New Operators A && B  A \n B

 *
 * return the intersection of A and B
*****************************************************************************/

PG_FUNCTION_INFO_V1(intset_intersec);

Datum
intset_intersec(PG_FUNCTION_ARGS)
{
	intSet * A;
	intSet * B;
    intSet * intersec_set;
    
    int searchStopIndex;
    int searchResult;

    int * intersection;
    int count;

    int i;
    int biggestNum;

    A = (intSet *) PG_GETARG_POINTER(0);
    B = (intSet *) PG_GETARG_POINTER(1);

    searchStopIndex = 0;
    searchResult = 0;

    count = 0;


    if(A->capacity > B->capacity){
        intersection = calloc(B->capacity, sizeof(int));
        // # 1 copy A to intersection
        memcpy(intersection, B->members, sizeof(int) * B->capacity);
        // # 2 Add new elements in B into union
        count = B->capacity;
        biggestNum = B->members[B->capacity - 1] + 1;

        for(i = 0; i < B->capacity; i++){
            searchResult = binarySearch(A->members, searchStopIndex, A->capacity - 1, intersection[i]);
            if(searchResult == -1){
                intersection[i] = biggestNum;
                count --;
                continue;
            }
            searchStopIndex = searchStopIndex > searchResult? searchStopIndex: searchResult;
        }
    }

    else{
        intersection = calloc(A->capacity, sizeof(int));
        // # 1 copy A to intersection
        memcpy(intersection, A->members, sizeof(int) * A->capacity);
        // # 2 Add new elements in B into union
        count = A->capacity;
        biggestNum = A->members[A->capacity - 1] + 1;

        for(i = 0; i < A->capacity; i++){
            searchResult = binarySearch(B->members, searchStopIndex, B->capacity - 1, intersection[i]);
            if(searchResult == -1){
                intersection[i] = biggestNum;
                count --;
                continue;
            }
            searchStopIndex = searchStopIndex > searchResult? searchStopIndex: searchResult;
        }
    }
    if(count)
        qsort(intersection, A->capacity < B->capacity? A->capacity : B->capacity, sizeof(int), cmpfunc);

    //initiate a new intSet
    intersec_set = palloc(VARHDRSZ + sizeof(int) *(count + 1));
    SET_VARSIZE(intersec_set, VARHDRSZ + sizeof(int) * (count + 1));
    intersec_set->capacity = count;
    // copy when #elements > 0
    if(count)
        memcpy(intersec_set->members, intersection, sizeof(int) * count);

    free(intersection);


    PG_RETURN_POINTER(intersec_set);
}




