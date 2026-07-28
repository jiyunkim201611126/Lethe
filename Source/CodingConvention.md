## 구조체 생성자

구조체 생성자를 정의해두면 구조체 객체 생성을 한 줄로 할 수 있어 코드 정리가 깔끔해보입니다.
하지만, 추후 구조체에 필드를 추가하게 되면 생성자를 사용했던 모든 코드를 찾아가 수정해야만 컴파일이 됩니다.
이걸 피하려면 새로운 생성자를 정의해야 하고, 구조체는 점점 길어져 읽기 싫게 됩니다.
생성자를 사용하지 않았다면 기본값을 정의해두는 것으로 컴파일도 가능하고 정상 동작까지 얻을 수 있습니다.
또한, 생성자는 변수 이름이 보이지 않으며 필드가 많아지면 너무 길어진다는 단점도 있습니다.

```c++
// 3이랑 10이 뭔데..
FMyStruct MyData(3, 10);

// Id가 3이고 Distance는 10이구나.
FMyStruct MyData;
MyData.Id = 3;
MyData.Distance = 10;

USTRUCT()
struct FMyStruct
{
    uint32 Id = 0;
    int32 Distance = 0;
    
    // 추가됨
    int32 NewValue = 0;
    
    // 기존 생성자
    //FMyStruct(const uint32 InId, const int32 InDistance) : Id(InId), Distance(InDistance) {}
    
    // 새로운 생성자, 벌써 길다.
    FMyStruct(const uint32 InId, const int32 InDistance, const int32 InNewValue) : Id(InId), Distance(InDistance), NewValue(InNewValue) {} 
};

// 기존 생성자를 지웠으니 이건 컴파일 에러
FMyStruct MyData(3, 10);

// 생성자로 만들 때 너무 길다. 만약 필드가 너무 많다면? 10개라면?
// FHitResult를 이런 식으로 초기화한다고 상상해보자.
FMyStruct MyData(MyDataId, CurrentDistance, ...);
```

---

## Add / Emplace 구분

Add는 삽입 과정에서 복사 연산이 발생할 수 있으며, Emplace는 컨테이너 내부에서 생성하며 집어넣습니다. 
'Emplace가 Add의 상위호환이네?' 싶은 생각이 드는 게 자연스러우나, 실제로는 사용해도 큰 차이를 보는 경우가 많지 않습니다.
그리고 모든 삽입 코드 작성을 Emplace로 해버리면, 다른 프로그래머들은 '이거 지금 컨테이너 안에서 생성하는 건가?' 하고 착각하는 경우가 발생할 수 있습니다.
특히 뒤에 _GetRef가 달려있다면 생성자 사용 없이 내부 필드를 초기화할 수 있으니 애용합시다.

Add - 이미 만들어진 값을 넣을 때, 혹은 복사 연산을 생략해도 이득이 크지 않은 경우

Emplace - 컨테이너 안에서 직접 생성할 의도가 있는 경우

```c++
int32 Index = 0;
MyArray.Add(Index);
MyMap.Add(Index);

FMyHeavyStruct& Data = MyArray.Emplace_GetRef();
Data.Value = 0;
FMyHeavyStruct& Data = MyMap.FindOrAdd(Key);
Data.Value = 0;
```

---

## 부등호 작성

보통 '검사 대상'을 왼쪽, '기준값'을 오른쪽에 둬 작성합니다.
그러나 둘 모두 변수인 경우 오름차순으로 정렬합니다.
만약 '범위 검사'라는 느낌이 강하다면 기준값도 오름차순 정렬에 포함시킵니다.

```c++
// 틀린 예시
0 < MyValue
MyValue1 >= MyValue2
MyValue > 0 && MyValue < 10

// 옳은 예시
MyValue > 0
MyValue1 <= MyValue2
0 < MyValue && MyValue < 10
```

---

## 접근지정자 순서

함수, 변수 순으로 작성하며, public, protected, private 순으로 작성합니다.
추가로 아래와 같은 주석은 부모 클래스의 함수를 region처럼 묶어 가독성을 향상시킵니다.
```c++
//~ Begin AActor Interface
//~ End of AActor Interface
```
반드시 있어야 하는 것은 아닙니다.
public 함수와 protected 함수를 모두 오버라이드하는 경우가 있다면 접근 지정자를 포함해 묶게 되므로,
가독성을 오히려 저하시킬 수 있어 이 경우는 주의해서 사용합니다.

---

## 변수명

자료형을 그대로 적기보다는 역할을 명시합니다.

```c++
// 틀린 예시
ULetheAbilitySystemComponent* LetheASC = Card->GetOwnerASC();

// 옳은 예시
ULetheAbilitySystemComponent* CardOwnerASC = Card->GetOwnerASC();
ULetheAbilitySystemComponent* SourceASC = MyCharacter->GetASC();
```

함수 구현을 통해 내부가 채워져서 나오는 인자는 앞에 Out을 붙여줍니다.

```c++
TArray<int32> OutMyValues;
FillMyValues(OutMyValues);
```
