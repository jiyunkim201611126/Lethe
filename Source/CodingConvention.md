## Add / Emplace 구분
Add는 삽입 과정에서 복사 연산이 발생하며, Emplace는 컨테이너 내부에서 생성하며 집어넣습니다. 
'Emplace가 Add의 상위호환이네?' 싶은 생각이 드는 게 자연스러우나, 실제로는 사용해도 큰 차이를 보는 경우가 많지 않습니다.
그리고 모든 삽입 코드 작성을 Emplace로 해버리면, 다른 프로그래머들은 '이거 지금 컨테이너 안에서 생성하는 건가?' 하고 착각하는 경우가 발생할 수 있습니다.

Add - 포인터나 일반 자료형 단순 추가, 복사 연산을 생략해도 이득이 크지 않은 경우

Emplace - 큰 구조체를 생성하며 삽입하는 경우

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

보통 '검사 대상'을 왼쪽, '기준값'을 오른쪽에 둬 작성합니다. 그러나 둘 모두 변수인 경우 오름차순으로 정렬합니다.

```c++
//~ 틀린 예시
0 < MyValue
MyValue1 >= MyValue2

//~ 옳은 예시
MyValue > 0
MyValue1 <= MyValue2
```