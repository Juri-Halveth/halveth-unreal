#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Base64.h"

// The complete input is retained by the UTF-8 resolver. All eight digest words
// participate in the field; this is a world recipe, not a numeric permission.
struct FVoidSoul {
    FString Text, ID;
    uint32 Words[8] = {};
    bool Load(const FString& File) {
        FString Raw; TSharedPtr<FJsonObject> Doc;
        if(!FFileHelper::LoadFileToString(Raw,*File) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Doc) || !Doc.IsValid()) return false;
        FString Schema, Recipe;
        if(!Doc->TryGetStringField(TEXT("schema"),Schema) || Schema!=TEXT("halveth-soulseed/1.0")) return false;
        if(!Doc->TryGetStringField(TEXT("recipe"),Recipe) || Recipe!=TEXT("sha256-domain-utf8-v1")) return false;
        if(!Doc->TryGetStringField(TEXT("text"),Text) || !Doc->TryGetStringField(TEXT("sha256"),ID) || ID.Len()!=64) return false;
        FString Encoded; double ByteLength=0; TArray<uint8> Bytes;
        if(!Doc->TryGetStringField(TEXT("utf8_base64"),Encoded) || !Doc->TryGetNumberField(TEXT("byte_length"),ByteLength)) return false;
        if(!FBase64::Decode(Encoded,Bytes) || FBase64::Encode(Bytes)!=Encoded || ByteLength!=Bytes.Num()) return false;
        FTCHARToUTF8 UTF8(*Text,Text.Len());
        if(UTF8.Length()!=Bytes.Num() || (Bytes.Num()>0 && FMemory::Memcmp(UTF8.Get(),Bytes.GetData(),Bytes.Num())!=0)) return false;
        for(TCHAR C:ID) if(!((C>=TEXT('0') && C<=TEXT('9')) || (C>=TEXT('a') && C<=TEXT('f')))) return false;
        for(int32 I=0;I<8;I++) Words[I]=uint32(FCString::Strtoui64(*ID.Mid(I*8,8),nullptr,16));
        return true;
    }
};
