//
//  ElementaryTypes.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "standard.h"
#include "Engine.hpp"
#include "String.h"
#include "List.h"
#include "String.h"
#include "Dictionary.h"
#include "../utf8.h"
#include "Thread.hpp"
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <algorithm>

namespace Emojicode {

EmojicodeInteger secureRandomNumber(EmojicodeInteger min, EmojicodeInteger max) {
    uint_fast64_t z;

#if defined __FreeBSD__ || defined __APPLE__ || defined __OpenBSD__ || defined __NetBSD__
    arc4random_buf(&z, sizeof(z));
#else
    FILE *fp = fopen("/dev/urandom", "r");
    fread(&z, 1, sizeof(z), fp);
    fclose(fp);
#endif

    return (z % (max - min + 1)) + min;
}

static void systemExit(Thread *thread, Value *destination) {
    EmojicodeInteger state = thread->getVariable(0).raw;
    exit((int)state);
}

static void systemGetEnv(Thread *thread, Value *destination) {
    char *env = getenv(stringToCString(thread->getVariable(0).object));

    if (!env) {
        destination->makeNothingness();
        return;
    }

    destination->optionalSet(stringFromChar(env));
}

static void systemCWD(Thread *thread, Value *destination) {
    char path[1050];
    getcwd(path, sizeof(path));

    destination->object = stringFromChar(path);
}

static void systemTime(Thread *thread, Value *destination) {
    destination->raw = time(nullptr);
}

static void systemArgs(Thread *thread, Value *destination) {
    Object *const &listObject = thread->retain(newObject(CL_LIST));

    List *newList = listObject->val<List>();
    newList->capacity = cliArgumentCount;
    Object *items = newArray(sizeof(Value) * cliArgumentCount);

    listObject->val<List>()->items = items;

    for (int i = 0; i < cliArgumentCount; i++) {
        listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT, stringFromChar(cliArguments[i]));
    }

    thread->release(1);
    destination->object = listObject;
}

static void systemSystem(Thread *thread, Value *destination) {
    FILE *f = popen(stringToCString(thread->getVariable(0).object), "r");

    if (!f) {
        destination->makeNothingness();
        return;
    }

    size_t bufferUsedSize = 0;
    int bufferSize = 50;
    Object *buffer = newArray(bufferSize);

    while (fgets(buffer->val<char>() + bufferUsedSize, bufferSize - (int)bufferUsedSize, f) != nullptr) {
        bufferUsedSize = strlen(buffer->val<char>());

        if (bufferSize - bufferUsedSize < 2) {
            bufferSize *= 2;
            buffer = resizeArray(buffer, bufferSize);
        }
    }

    bufferUsedSize = strlen(buffer->val<char>());

    EmojicodeInteger len = u8_strlen_l(buffer->val<char>(), bufferUsedSize);

    Object *const &so = thread->retain(newObject(CL_STRING));
    String *string = so->val<String>();
    string->length = len;

    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = so->val<String>();
    string->charactersObject = chars;

    u8_toucs(string->characters(), len, buffer->val<char>(), bufferUsedSize);
    thread->release(1);
    destination->object = so;
}

//MARK: Threads

static void threadJoin(Thread *thread, Value *destination) {
    allowGC();
    auto cthread = thread->getThisObject()->val<std::thread>();
    cthread->join();  // TODO: GC?!
    disallowGCAndPauseIfNeeded();
}

static void threadSleepMicroseconds(Thread *thread, Value *destination) {
    std::this_thread::sleep_for(std::chrono::microseconds(thread->getVariable(0).raw));
}

void threadStart(Object *callable) {
    Thread thread = Thread();
    executeCallableExtern(callable, nullptr, 0, &thread, nullptr);
}

static void initThread(Thread *thread, Value *destination) {
    *thread->getThisObject()->val<std::thread>() = std::thread(threadStart, thread->getVariable(0).object);
    *destination = thread->getThisContext();
}

static void initMutex(Thread *thread, Value *destination) {
    pthread_mutex_init(thread->getThisObject()->val<pthread_mutex_t>(), nullptr);
    *destination = thread->getThisContext();
}

static void mutexLock(Thread *thread, Value *destination) {
    while (pthread_mutex_trylock(thread->getThisObject()->val<pthread_mutex_t>()) != 0) {
        // TODO: Obviously stupid, but this is the only safe way. If pthread_mutex_lock was used,
        // the thread would be block, and the GC could cause a deadlock. allowGC, however, would
        // allow moving this mutex – obviously not a good idea either when using pthread_mutex_lock.
        pauseForGC();
        usleep(10);
    }
}

static void mutexUnlock(Thread *thread, Value *destination) {
    pthread_mutex_unlock(thread->getThisObject()->val<pthread_mutex_t>());
}

static void mutexTryLock(Thread *thread, Value *destination) {
    destination->raw = pthread_mutex_trylock(thread->getThisObject()->val<pthread_mutex_t>()) == 0;
}

// MARK: Data

static void dataEqual(Thread *thread, Value *destination) {
    Data *d = thread->getThisObject()->val<Data>();
    Data *b = thread->getVariable(0).object->val<Data>();

    if (d->length != b->length) {
        destination->raw = 0;
        return;
    }

    destination->raw = memcmp(d->bytes, b->bytes, d->length) == 0;
}

static void dataSize(Thread *thread, Value *destination) {
    destination->raw = thread->getThisObject()->val<Data>()->length;
}

static void dataMark(Object *o) {
    Data *d = o->val<Data>();
    if (d->bytesObject) {
        mark(&d->bytesObject);
        d->bytes = d->bytesObject->val<char>();
    }
}

static void dataGetByte(Thread *thread, Value *destination) {
    Data *d = thread->getThisObject()->val<Data>();

    EmojicodeInteger index = thread->getVariable(0).raw;
    if (index < 0) {
        index += d->length;
    }
    if (index < 0 || d->length <= index) {
        destination->makeNothingness();
        return;
    }

    destination->optionalSet(EmojicodeInteger(d->bytes[index]));
}

static void dataToString(Thread *thread, Value *destination) {
    Data *data = thread->getThisObject()->val<Data>();
    if (!u8_isvalid(data->bytes, data->length)) {
        destination->makeNothingness();
        return;
    }

    EmojicodeInteger len = u8_strlen_l(data->bytes, data->length);
    Object *const &characters = thread->retain(newArray(len * sizeof(EmojicodeChar)));

    Object *sto = newObject(CL_STRING);
    String *string = sto->val<String>();
    string->length = len;
    string->charactersObject = characters;
    thread->release(1);
    u8_toucs(string->characters(), len, data->bytes, data->length);
    destination->optionalSet(sto);
}

static void dataSlice(Thread *thread, Value *destination) {
    Object *ooData = newObject(CL_DATA);
    Data *oData = ooData->val<Data>();
    Data *data = thread->getThisObject()->val<Data>();

    EmojicodeInteger from = thread->getVariable(0).raw;
    if (from >= data->length) {
        destination->object = ooData;
        return;
    }

    EmojicodeInteger l = thread->getVariable(1).raw;
    if (thread->getVariable(0).raw + l > data->length) {
        l = data->length - thread->getVariable(0).raw;
    }

    oData->bytesObject = data->bytesObject;
    oData->bytes = data->bytes + from;
    oData->length = l;
    destination->object = ooData;
}

static void dataIndexOf(Thread *thread, Value *destination) {
    Data *data = thread->getThisObject()->val<Data>();
    Data *search = thread->getVariable(0).object->val<Data>();
    auto last = data->bytes + data->length;
    const void *location = std::search(data->bytes, last, search->bytes, search->bytes + search->length);
    if (location == last) {
        destination->makeNothingness();
    }
    else {
        destination->optionalSet(EmojicodeInteger((Byte *)location - (Byte *)data->bytes));
    }
}

static void dataByAppendingData(Thread *thread, Value *destination) {
    Data *data = thread->getThisObject()->val<Data>();
    Data *b = thread->getVariable(0).object->val<Data>();

    size_t size = data->length + b->length;
    Object *const &newBytes = thread->retain(newArray(size));

    b = thread->getVariable(0).object->val<Data>();
    data = thread->getThisObject()->val<Data>();

    std::memcpy(newBytes->val<char>(), data->bytes, data->length);
    std::memcpy(newBytes->val<char>() + data->length, b->bytes, b->length);

    Object *ooData = newObject(CL_DATA);
    Data *oData = ooData->val<Data>();
    oData->bytesObject = newBytes;
    oData->bytes = oData->bytesObject->val<char>();
    oData->length = size;
    thread->release(1);
    destination->object = ooData;
}

// MARK: Integer

void integerToString(Thread *thread, Value *destination) {
    EmojicodeInteger base = thread->getVariable(0).raw;
    EmojicodeInteger n = thread->getThisContext().value->raw, a = std::abs(n);
    bool negative = n < 0;

    EmojicodeInteger d = negative ? 2 : 1;
    while (n /= base) d++;

    Object *const &co = thread->retain(newArray(d * sizeof(EmojicodeChar)));

    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->val<String>();
    string->length = d;
    string->charactersObject = co;

    EmojicodeChar *characters = string->characters() + d;
    do
        *--characters =  "0123456789abcdefghijklmnopqrstuvxyz"[a % base % 35];
    while (a /= base);

    if (negative) characters[-1] = '-';
    thread->release(1);
    destination->object = stringObject;
}

static void integerRandom(Thread *thread, Value *destination) {
    thread->getThisContext().value->raw = secureRandomNumber(thread->getVariable(0).raw, thread->getVariable(1).raw);
}

static void integerAbsolute(Thread *thread, Value *destination) {
    destination->raw = std::abs(thread->getThisContext().value->raw);
}

static void symbolToString(Thread *thread, Value *destination) {
    Object *co = thread->retain(newArray(sizeof(EmojicodeChar)));
    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->val<String>();
    string->length = 1;
    string->charactersObject = co;
    thread->release(1);
    string->characters()[0] = thread->getThisContext().value->character;
    destination->object = stringObject;
}

static void symbolToInteger(Thread *thread, Value *destination) {
    destination->raw = thread->getThisContext().value->character;
}

static void doubleToString(Thread *thread, Value *destination) {
    EmojicodeInteger precision = thread->getVariable(0).raw;
    double d = thread->getThisContext().value->doubl;
    double absD = std::abs(d);

    bool negative = d < 0;

    EmojicodeInteger length = negative ? 1 : 0;
    if (precision != 0) {
        length++;
    }
    length += precision;
    EmojicodeInteger iLength = 1;
    for (size_t i = 1; pow(10, i) < absD; i++) {
        iLength++;
    }
    length += iLength;

    Object *const &co = thread->retain(newArray(length * sizeof(EmojicodeChar)));
    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->val<String>();
    string->length = length;
    string->charactersObject = co;
    thread->release(1);
    EmojicodeChar *characters = string->characters() + length;

    for (size_t i = precision; i > 0; i--) {
        *--characters =  (unsigned char) (fmod(absD * pow(10, i), 10.0)) % 10 + '0';
    }

    if (precision != 0) {
        *--characters = '.';
    }

    for (size_t i = 0; i < iLength; i++) {
        *--characters =  (unsigned char) (fmod(absD / pow(10, i), 10.0)) % 10 + '0';
    }

    if (negative) characters[-1] = '-';
    destination->object = stringObject;
}

static void doubleSin(Thread *thread, Value *destination) {
    destination->doubl = sin(thread->getThisContext().value->doubl);
}

static void doubleCos(Thread *thread, Value *destination) {
    destination->doubl = cos(thread->getThisContext().value->doubl);
}

static void doubleTan(Thread *thread, Value *destination) {
    destination->doubl = tan(thread->getThisContext().value->doubl);
}

static void doubleASin(Thread *thread, Value *destination) {
    destination->doubl = asin(thread->getThisContext().value->doubl);
}

static void doubleACos(Thread *thread, Value *destination) {
    destination->doubl = acos(thread->getThisContext().value->doubl);
}

static void doubleATan(Thread *thread, Value *destination) {
    destination->doubl = atan(thread->getThisContext().value->doubl);
}

static void doublePow(Thread *thread, Value *destination) {
    destination->doubl = pow(thread->getThisContext().value->doubl, thread->getVariable(0).doubl);
}

static void doubleSqrt(Thread *thread, Value *destination) {
    destination->doubl = sqrt(thread->getThisContext().value->doubl);
}

static void doubleRound(Thread *thread, Value *destination) {
    destination->raw = static_cast<EmojicodeInteger>(round(thread->getThisContext().value->doubl));
}

static void doubleCeil(Thread *thread, Value *destination) {
    destination->raw = static_cast<EmojicodeInteger>(ceil(thread->getThisContext().value->doubl));
}

static void doubleFloor(Thread *thread, Value *destination) {
    destination->raw = static_cast<EmojicodeInteger>(floor(thread->getThisContext().value->doubl));
}

static void doubleLog2(Thread *thread, Value *destination) {
    destination->doubl = log2(thread->getThisContext().value->doubl);
}

static void doubleLn(Thread *thread, Value *destination) {
    destination->doubl = log(thread->getThisContext().value->doubl);
}

static void doubleAbsolute(Thread *thread, Value *destination) {
    destination->doubl = fabs(thread->getThisContext().value->doubl);
}

// MARK: Callable

static void closureMark(Object *o) {
// TODO: Add correct implementation!
//    Closure *c = static_cast<Closure *>(o->value);
//    if (isRealObject(c->thisContext)) {
//        mark(&c->thisContext.object);
//    }
//    mark(&c->capturedVariables);
//
//    Value *t = static_cast<Value *>(c->capturedVariables->value);
//    CaptureInformation *infop = static_cast<CaptureInformation *>(c->capturesInformation->value);
//    for (unsigned int i = 0; i < c->captureCount; i++) {
//        Value *s = t + (infop++)->destination;
//        if (isRealObject(*s)) {
//            mark(&s->object);
//        }
//    }
}

FunctionFunctionPointer sLinkingTable[] = {
    nullptr,
    //📇
    dataEqual,
    dataSize,  // 🐔
    dataGetByte,  // 🐽
    dataToString,  // 🔡
    dataSlice,  // 🔪
    dataIndexOf,  // 🔍
    dataByAppendingData,  // 📝
    //💈
    initThread,
    nullptr,
    threadSleepMicroseconds,  // ⏲
    threadJoin,  // 🛂
    //🔐
    initMutex,
    mutexLock,  // 🔒
    mutexUnlock,  // 🔓
    mutexTryLock,  // 🔐
    //🚂
    integerToString,  // 🔡
    integerRandom,  // 🎰
    integerAbsolute,  // 🏧
    doubleSin,  //📓
    doubleCos,  //📕
    doubleTan,  //📘
    doubleASin,  //📔
    doubleACos,  //📙
    doubleATan,  //📗
    doublePow,  //🏂
    doubleSqrt,  //⛷
    doubleCeil,  //🚴
    doubleFloor,  //🚵
    doubleRound,  //🏇
    doubleLog2,  //🚣
    doubleLn,  //🏄
    doubleToString,  //🔡
    doubleAbsolute,  //🏧
    //🔣
    symbolToString,  //🔡
    symbolToInteger,  //🚂
    //💻
    systemExit,  //🚪
    systemGetEnv,  //🌳
    systemCWD,
    systemTime,  //🕰
    systemArgs,  //🎞
    systemSystem,  //🕴
    nullptr,
    nullptr,
    nullptr,
    listAppendBridge,  // bear
    listGetBridge,  //🐽
    listRemoveBridge,  // koala
    listInsertBridge,  // monkey
    listCountBridge,  //🐔
    listPopBridge,  // panda
    listShuffleInPlaceBridge,  //🐹
    listFromListBridge,  //🐮
    listSort,  //🦁
    listRemoveAllBridge,  //🐗
    listSetBridge,  //🐷
    listEnsureCapacityBridge,  //🐴
    initListWithCapacity,  //🐧
    initListEmptyBridge,
    stringPrintStdoutBrigde,
    stringEqualBridge,  //🐔
    stringLengthBridge,  //📝
    stringByAppendingSymbolBridge,  //🐽
    stringSymbolAtBridge,  //🔪
    stringSubstringBridge,  //🔍
    stringIndexOf,  //🔧
    stringTrimBridge,  //🔫
    stringSplitByStringBridge,  //📐
    stringUTF8LengthBridge,  //💣
    stringSplitBySymbolBridge,  //🎼
    stringBeginsWithBridge,  //⛳️
    stringEndsWithBridge,  //🎶
    stringToCharacterList,  //📇
    stringToData,  //📰
    stringJSON,  //🚂
    stringToInteger,  //🚀
    stringToDouble,  //↔️
    stringCompareBridge,  //📫
    stringToUppercase,  //📪
    stringToLowercase,
    stringGetInput,  //😮
    stringFromSymbolListBridge,  //🎙
    stringFromStringList,  //🍨
    nullptr,
    nullptr,
    nullptr,
    initDictionaryBridge,
    bridgeDictionaryGet,  //🐽
    bridgeDictionaryRemove,  //🐨
    bridgeDictionarySet,  //🐷
    bridgeDictionaryKeys,  //🐙
    bridgeDictionaryClear,  //🐗
    bridgeDictionaryContains,  //🐣
    bridgeDictionarySize,  //🐔
};

uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name) {
    switch (name) {
        case 0x1F521:
            return sizeof(String);
        case 0x1F368:
            return sizeof(List);
        case 0x1F36F:
            return sizeof(EmojicodeDictionary);
        case 0x1F4C7:
            return sizeof(Data);
        case 0x1F347:
            return sizeof(Closure);
        case 0x1f488:  //💈
            return sizeof(std::thread);
        case 0x1f510:  //🔐
            return sizeof(pthread_mutex_t);
    }
    return 0;
}

Marker markerPointerForClass(EmojicodeChar cl) {
    switch (cl) {
        case 0x1F368:  // List
            return listMark;
        case 0x1F36F:  // Dictionary
            return dictionaryMark;
        case 0x1F521:
            return stringMark;
        case 0x1F347:
            return closureMark;
        case 0x1F4C7:
            return dataMark;
    }
    return nullptr;
}

}
