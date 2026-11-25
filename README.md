# FAT32Recovery 프로젝트 최신 인수인계 문서

**프로젝트명:** FAT32Recovery\
**개발 언어:** C++17\
**개발 대상:** 손상된 SD카드에서 덤프한 BIN 이미지\
**기반 저장소:** https://github.com/thehylee/MYAPP\_01\
**테스트 이미지:** 4gb.bin\
**핵심 원칙:** 원본 이미지 읽기 전용 분석 및 데이터 복원

---

# 1\. 프로젝트 개발 목적

손상된 SD카드에서 생성한 BIN 이미지 파일을 분석하여 FAT32 파일시스템 구조와 실제 저장 데이터를 최대한 복원하는 것이 목적이다.

다음 두 가지 방식을 병행한다.

1. FAT32 파일시스템 기반 복원
2. FAT 파일시스템에 의존하지 않는 RAW 데이터 카빙

최종적으로 다음 데이터를 복원하고 분석 근거를 보존한다.

- 기존 디렉터리 구조
- 기존 폴더 및 파일 이름
- SFN 및 LFN 엔트리
- 디렉터리 엔트리 위치
- 파일 시작 클러스터
- FAT1 및 FAT2 연결 관계
- 정상 파일
- 삭제 파일
- 손상 파일
- 고아 클러스터
- 단편화된 데이터
- 파일 무결성 및 SHA-256

**확인되지 않은 구조를 임의로 가정하거나 수정하지 않는다.**

---

# 2\. GitHub 프로젝트 관리

기존 저장소:

https://github.com/thehylee/MYAPP\_01

기존 소스는 신규 개발의 기반으로 사용한다.

개발 규칙:

1. 신규 코드 작성 전에 관련 GitHub 소스의 실제 인터페이스를 확인한다.
2. 기존 모듈을 불필요하게 다시 작성하지 않는다.
3. 기존 함수명, 자료형, 멤버 변수 및 인터페이스를 확인한다.
4. 존재하지 않는 함수나 변수를 임의로 사용하지 않는다.
5. 인터페이스 변경이 필요하면 변경 이유와 영향을 먼저 설명한다.
6. main.cpp와 FileExtractor는 참고용이며 필요하면 새롭게 설계할 수 있다.
7. 기존 모듈과 중복 선언이 발생하지 않도록 확인한다.

현재 대화에서 개별 GitHub 소스의 최신 구현을 모두 확보한 것은 아니다. 실제 코드를 작성하기 전에 반드시 확인한다.

---

# 3\. 현재 모듈 개발 상태

| 모듈 | 상태 |
| --- | --- |
| DiskImage | 구현 완료 |
| BPBParser | 구현 완료 |
| FATReader | 구현 완료 |
| FATAnalyzer | 구현 완료 |
| FATChainAnalyzer | 구현 완료 |
| DirectoryScanner | 기본 구현 완료 |
| LFNParser | 기본 구현 완료 |
| TreeBuilder | 구현 및 확장 완료 |
| DirectoryTraversal | 신규 개발 예정 |
| FileExtractor | 신규 구현 예정 |
| DeletedFileScanner | 미구현 |
| OrphanScanner | 미구현 |
| FileCarver | 미구현 |
| FragmentAnalyzer | 미구현 |
| FileValidator | 미구현 |
| CSVWriter | 미구현 |
| ReportWriter | 미구현 |

---

# 4\. 테스트 이미지 4gb.bin 정보

## 4.1 이미지 기본 정보

| 항목 | 값 |
| --- | --- |
| 파일명 | 4gb.bin |
| 이미지 크기 | 4,027,580,416 bytes |
| 전체 섹터 | 7,866,368 |
| 섹터 크기 | 512 bytes |
| 전체 용량 | 약 3.75 GiB |
| 나머지 바이트 | 0 |

이미지는 정상 SD카드가 아니라 손상된 SD카드에서 덤프한 파일이다.

원본 이미지는 읽기 전용으로 취급한다.

## 4.2 MBR 파티션 정보

text

```text
=== MBR PARTITIONS ===

Partition 1 Type=0xc Start=128 Size=7866240
Partition 2 Type=0x0 Start=0 Size=0
Partition 3 Type=0x0 Start=0 Size=0
Partition 4 Type=0x0 Start=0 Size=0
```

Partition 1은 FAT32 LBA 파티션이다.

파티션 마지막 섹터:

text

```text
128 + 7866240 - 1 = 7866367
```

전체 이미지 마지막 섹터:

text

```text
7866368 - 1 = 7866367
```

파티션 끝과 이미지 끝이 일치한다.

MBR Signature:

text

```text
55 AA
```

## 4.3 BPB 정보

주 분석 대상은 MBR에서 지정한 LBA 128의 Primary VBR이다.

text

```text
OEM: MSDOS5.0

Bytes/Sector: 512
Sectors/Cluster: 8
Reserved Sectors: 1052

Number of FATs: 2
Total Sectors: 7866240
FAT Size: 7666

Root Cluster: 2
FSInfo Sector: 1
Backup Sector: 6

Hidden Sectors: 128
FS Version: 0

Signature: 55 AA
```

계산 결과:

| 항목 | 값 |
| --- | --- |
| Partition Start | 128 |
| Reserved Sectors | 1052 |
| FAT1 Start Absolute | 1180 |
| FAT2 Start Absolute | 8846 |
| Data Start Relative | 16384 |
| Data Start Absolute | 16512 |
| Cluster Size | 4096 bytes |
| Cluster Count | 981232 |
| Maximum Cluster | 981233 |
| FAT Entries | 981248 |

BPB Validation:

text

```text
PASSED
```

## 4.4 Primary VBR와 Backup VBR

BPB Candidates 결과:

text

```text
Candidate LBA: 128
OEM: MSDOS5.0
Bytes/Sector: 512
Sectors/Cluster: 8
Reserved: 1052
FAT Count: 2
Total Sectors: 7866240
FAT Size: 7666
Root Cluster: 2
FSInfo Sector: 1
Backup Sector: 6
Signature: 55 aa
```

두 번째 후보:

text

```text
Candidate LBA: 134
OEM: MSDOS5.0
Bytes/Sector: 512
Sectors/Cluster: 8
Reserved: 1052
FAT Count: 2
Total Sectors: 7866240
FAT Size: 7666
Root Cluster: 2
FSInfo Sector: 1
Backup Sector: 6
Signature: 55 aa
```

중요한 사실:

text

```text
Partition Start = 128
Backup Boot Sector = 6

128 + 6 = 134
```

따라서 다음과 같이 분류한다.

- LBA 128: Primary VBR
- LBA 134: Backup VBR

향후 BPBParser에서 두 BPB의 주요 필드를 비교하는 기능을 추가한다.

두 영역의 값이 일치한다고 미리 가정하지 않는다.

---

# 5\. FAT1 및 FAT2 분석 결과

## 5.1 초기 FAT 엔트리 비교

| Cluster | FAT1 | FAT2 |
| --- | --- | --- |
| 0 | 0x0FFFFFF8 | 0 |
| 1 | 0xFFFFFFFF | 0 |
| 2 | 0x5 | 0 |
| 3 | 0x0FFFFFFF | 0 |
| 4 | 0x0FFFFFFF | 0 |
| 5 | 0x0FFFFFFF | 0 |
| 6 | 0x0FFFFFFF | 0 |
| 7 | 0x0FFFFFFF | 0 |
| 8 | 0x0FFFFFFF | 0 |
| 9 | 0 | 0 |
| 10 | 0 | 0 |
| 11 | 0 | 0 |
| 12 | 0 | 0 |
| 13 | 0 | 0 |
| 14 | 0 | 0 |
| 15 | 0 | 0 |

위 값은 제공된 초기 FAT 엔트리 분석 결과다.

## 5.2 FAT1 통계

기존 FATAnalyzer 출력:

text

```text
FREE      : 981225
BAD       : 0
EOC       : 6
RESERVED  : 0
ALLOCATED : 1
INVALID   : 0
```

Root Cluster:

text

```text
Cluster 2 -> Cluster 5 -> END
```

FAT1 Cluster 2:

text

```text
Raw: 0x00000005
Value: 0x00000005
Type: NEXT_CLUSTER
```

FAT1 Cluster 5:

text

```text
Raw: 0x0FFFFFFF
Value: 0x0FFFFFFF
Type: END_OF_CHAIN
```

## 5.3 FAT2 통계

기존 FATAnalyzer 출력:

text

```text
FREE      : 956516
BAD       : 0
EOC       : 248
RESERVED  : 15
ALLOCATED : 21403
INVALID   : 3050
```

추가 분석:

text

```text
Allocated Entries : 21403
EOC Entries       : 248
Invalid Entries   : 3065
Chain Start Candidates: 21396
```

주의:

FAT2 Invalid 통계는 서로 다른 출력에서 3050과 3065로 나타난다.

이 차이는 분석 범위 또는 집계 방식 차이일 수 있으므로 임의로 통일하지 않는다.

향후 FATAnalyzer 통계 구현을 확인한다.

## 5.4 FAT 비교

기존 FATAnalyzer 출력:

text

```text
Identical: 956523
Different: 24725
```

다른 기존 분석에서는 다음 값도 출력되었다.

text

```text
Matching Entries: 956509
Mismatching Entries: 24723
Invalid Entries: 3050
```

이 수치들은 서로 다른 실행 결과로 보존한다.

동일한 실행 결과라고 가정하거나 임의로 수정하지 않는다.

## 5.5 Root Cluster FAT 비교

text

```text
FAT1 Cluster 2: 0x5
FAT2 Cluster 2: 0x0
```

판정:

- FAT1: NEXT\_CLUSTER
- FAT2: FREE

Root Cluster 2의 FAT1 체인:

text

```text
2 → 5 → END
```

FAT2 체인:

text

```text
2 → FREE
```

현재 어느 FAT가 올바른지 확정하지 않는다.

FAT 미러링 설정 및 활성 FAT 설정도 확인해야 한다.

---

# 6\. Root Directory 분석

## 6.1 Root Cluster 위치

BPB 계산 결과:

text

```text
Data Start Absolute Sector: 16512
Cluster Size: 4096
Root Cluster: 2
```

FAT32 클러스터 오프셋 계산식:

cpp

```cpp
offset = dataStartOffset
       + (clusterNumber - 2) * clusterSize;
```

Root Cluster 2:

text

```text
16512 * 512 = 8454144

HEX: 0x810000
```

계산 결과:

| Cluster | Offset Decimal | Offset HEX |
| --- | --- | --- |
| 2 | 8454144 | 0x810000 |
| 5 | 8466432 | 0x813000 |
| 6 | 8470528 | 0x814000 |

## 6.2 중요 오프셋 불일치

이전 실행 결과:

text

```text
Root Offset: 8519680
HEX: 0x820000
```

이번 계산 결과:

text

```text
Root Offset: 8454144
HEX: 0x810000
```

차이:

text

```text
65536 bytes
```

이 문제는 아직 원인이 확정되지 않았다.

다음 항목을 확인해야 한다.

1. Data Start 계산
2. Cluster Number 계산
3. Partition Start 중복 적용
4. DiskImage의 Read 함수 오프셋 기준
5. Root Offset 계산에 사용된 변수
6. Sector Offset과 Byte Offset 혼용 여부

이 문제를 해결하기 전까지 DirectoryScanner와 FileExtractor의 위치 계산을 확정하지 않는다.

## 6.3 Root Cluster RAW 데이터

Cluster 2:

text

```text
Offset: 0x810000

FF FF FF FF FF FF FF FF
FF FF FF FF FF FF FF FF
FF FF FF FF FF FF FF FF
FF FF FF FF FF FF FF FF
```

기존 Root Directory Scanner에서 다음 결과가 출력되었다.

text

```text
FIRST BYTE : 0xFF
ATTRIBUTE  : 0xFF
```

이후 24개 엔트리가 모두 다음과 같이 출력되었다.

text

```text
Name: Empty
Cluster: 0
Size: 0
Deleted: NO
```

이 데이터는 일반적인 FAT32 디렉터리 엔트리로 해석할 수 없는 상태다.

`0xFF`를 정상 엔트리 또는 삭제 엔트리로 임의 분류하지 않는다.

## 6.4 Cluster 5 RAW 데이터

text

```text
Offset: 0x813000
```

초기 데이터:

text

```text
42 20 00 49 00 6E 00 66 00 6F 00 0F 00 72
72 00 6D 00 61 00 74 00 69 00 6F 00 00 00
6E 00 00 00

01 53 00 79 00 73 00 74 00 65 00 0F 00 72
6D 00 20 00 56 00 6F 00 6C 00 75 00 00 00
6D 00 65 00
```

기존 DirectoryScanner 결과:

text

```text
Offset: 0x813000 | LFN | Cluster: 0 | Size: 0
Offset: 0x813020 | LFN | Cluster: 0 | Size: 0
Offset: 0x813040 | SYSTEM~1 | Cluster: 6 | Size: 0
```

확인된 이름:

text

```text
SYSTEM~1
System Volume Information
```

LFN 문자열 연결 및 SFN 매핑은 기존 LFNParser 구현을 기준으로 추가 검증한다.

## 6.5 Cluster 6 RAW 데이터

text

```text
Offset: 0x814000
```

확인된 데이터:

text

```text
2E 20 20 20 20 20 20 20 20 20 20 10 ...
2E 2E 20 20 20 20 20 20 20 20 20 10 ...
```

이는 FAT32 디렉터리의 다음 엔트리에 해당한다.

- `.` : 현재 디렉터리
- `..` : 부모 디렉터리

Cluster 6은 하위 디렉터리 엔트리를 포함하는 것으로 확인되었다.

다만 이 디렉터리 안에 일반 파일이 존재한다는 사실은 아직 확인되지 않았다.

## 6.6 현재 확인된 디렉터리 구조

text

```text
ROOT /
  Cluster 2
    |
    +-- Cluster 5
          |
          +-- System Volume Information
                SFN: SYSTEM~1
                Cluster 6
                  |
                  +-- .
                  +-- ..
```

이 구조는 현재 확보한 RAW 데이터 및 FAT1 연결 관계를 기준으로 한 분석 결과다.

확정된 전체 파일시스템 구조로 간주하지 않는다.

---

# 7\. 기존 FileExtractor 테스트 결과

기존 참고용 FileExtractor 실행 결과:

| 파일 | 시작 클러스터 | 파일 크기 | 결과 |
| --- | --- | --- | --- |
| 파일 1 | 77911502 | 3975811057 | INVALID\_CLUSTER |
| 파일 2 | 30366697 | 1097822544 | INVALID\_CLUSTER |

결과:

text

```text
Total Files: 2
Success: 0
Partial: 0
Failed: 2
Extracted Bytes: 0
```

이 결과만으로 파일시스템의 실제 오류 원인을 확정하지 않는다.

기존 TreeBuilder에서 출력된 시작 클러스터와 FileExtractor에서 출력된 시작 클러스터가 달랐다는 기록이 있다.

이 차이는 전체 모듈 구현 완료 후 검증한다.

FileExtractor는 필요하다면 신규 설계한다.

---

# 8\. 현재 가장 중요한 기술적 문제

## 8.1 오프셋 계산 검증

최우선 확인 사항이다.

현재 BPB 계산 결과:

text

```text
Root Cluster 2
Expected Offset 8454144
Expected HEX 0x810000
```

기존 로그:

text

```text
Root Offset 8519680
HEX 0x820000
```

이 차이를 설명할 수 있는 기존 코드와 실제 이미지 RAW 데이터를 대조해야 한다.

오프셋 계산 오류가 존재한다면 다음 모든 모듈에 영향을 줄 수 있다.

- DirectoryScanner
- TreeBuilder
- FileExtractor
- DeletedFileScanner
- OrphanScanner
- FileCarver

따라서 신규 복구 모듈을 작성하기 전에 먼저 검증한다.

## 8.2 Primary/Backup VBR 비교

LBA 128과 LBA 134는 각각 Primary VBR 및 Backup VBR 후보이다.

BPBParser에 다음 분석을 추가하는 것을 권장한다.

- Bytes Per Sector 비교
- Sectors Per Cluster 비교
- Reserved Sector 비교
- FAT Count 비교
- Total Sectors 비교
- FAT Size 비교
- Root Cluster 비교
- FSInfo Sector 비교
- Backup Sector 비교
- Boot Signature 비교

비교 결과는 보고서로 기록하고 어느 한쪽을 자동으로 덮어쓰지 않는다.

## 8.3 FAT1/FAT2 독립 분석

두 FAT를 무조건 하나로 통합하지 않는다.

각각의 원본 값과 체인 결과를 보존한다.

활성 FAT 설정 및 FAT mirroring 설정을 확인한다.

FAT1과 FAT2의 불일치는 별도 분석 결과로 기록한다.

## 8.4 FAT 통계 범위 구분

다음 두 가지 통계를 구분한다.

1. FAT 전체 엔트리 통계
2. 실제 데이터 클러스터 통계

FAT 전체 엔트리는 981248개이고 데이터 클러스터 수는 981232개다.

서로 다른 집계 범위의 결과를 직접 비교하지 않는다.

---

# 9\. 향후 개발 순서

기존 개발 우선순위를 유지하되 오프셋 검증 작업을 먼저 진행한다.

| 순서 | 개발 항목 | 중요도 |
| --- | --- | --- |
| 1 | 기존 인터페이스 확인 | 매우 높음 |
| 2 | DiskImage 오프셋 검증 | 매우 높음 |
| 3 | BPBParser Primary/Backup 비교 | 높음 |
| 4 | FATReader 검증 | 매우 높음 |
| 5 | FATChainAnalyzer 연동 | 매우 높음 |
| 6 | DirectoryTraversal | 높음 |
| 7 | FileExtractor 신규 구현 | 매우 높음 |
| 8 | DeletedFileScanner | 높음 |
| 9 | OrphanScanner | 높음 |
| 10 | FileCarver | 높음 |
| 11 | FragmentAnalyzer | 높음 |
| 12 | FileValidator | 매우 높음 |
| 13 | CSVWriter | 높음 |
| 14 | ReportWriter | 높음 |

각 모듈은 독립적으로 구현하고 테스트한 후 다음 단계로 이동한다.

---

# 10\. 신규 FileExtractor 설계 요구사항

FileExtractor는 기존 구현을 참고하되 필요하면 새롭게 설계한다.

필수 기능:

1. 정상 파일 추출
2. FATChainAnalyzer 결과 활용
3. 파일 크기에 따른 데이터 추출
4. FAT1/FAT2 체인 결과 분리
5. FAT 미러링 설정 고려
6. 활성 FAT 설정 고려
7. FAT 불일치 처리
8. 순환 체인 탐지
9. INVALID\_CLUSTER 처리
10. FREE\_CLUSTER 처리
11. BAD\_CLUSTER 처리
12. EOC 처리
13. 이미지 읽기 오류 처리
14. 출력 파일 쓰기 오류 처리
15. Partial Extraction 기록
16. 원본 엔트리 오프셋 보존
17. 추출 결과 기록

파일 추출 결과에는 다음 정보를 보존한다.

- 원본 파일명
- 원본 경로
- 시작 클러스터
- 파일 크기
- 디렉터리 엔트리 오프셋
- 사용한 FAT
- 체인 상태
- 추출 바이트 수
- 추출 성공 여부
- SHA-256
- 오류 사유

FAT1과 FAT2 중 하나를 무조건 선택하지 않는다.

체인에 불확실성이 있으면 그 사실을 기록한다.

---

# 11\. 신규 DirectoryTraversal 설계 요구사항

기존 DirectoryScanner와 TreeBuilder를 최대한 활용한다.

신규 파일을 추가하는 방식으로 구현하는 것을 우선 검토한다.

예상 파일:

text

```text
Directory/
  DirectoryTraversal.h
  DirectoryTraversal.cpp
```

필수 기능:

1. Root Directory부터 시작
2. FAT 체인 분석 결과 활용
3. 하위 디렉터리 재귀 탐색
4. `.` 및 `..` 제외
5. 방문 클러스터 기록
6. 순환 참조 방지
7. LFN/SFN 연결
8. 원본 엔트리 오프셋 보존
9. FAT1/FAT2 탐색 결과 분리
10. 잘못된 체인 상태 기록
11. 최대 탐색 깊이 제한
12. 동일 디렉터리 중복 방문 방지

FAT 체인이 FREE, INVALID, BAD 또는 순환 상태이면 정상 체인으로 간주하지 않는다.

---

# 12\. 전체 프로젝트 디렉터리 구조

text

```text
FAT32Recovery/
│
├── 4gb.bin
├── main.cpp
│
├── Core/
│   ├── DiskImage.h
│   └── DiskImage.cpp
│
├── Filesystem/
│   ├── BPBParser.h
│   ├── BPBParser.cpp
│   ├── FATReader.h
│   ├── FATReader.cpp
│   ├── FATAnalyzer.h
│   ├── FATAnalyzer.cpp
│   ├── FATChainAnalyzer.h
│   └── FATChainAnalyzer.cpp
│
├── Directory/
│   ├── DirectoryScanner.h
│   ├── DirectoryScanner.cpp
│   ├── LFNParser.h
│   ├── LFNParser.cpp
│   ├── TreeBuilder.h
│   ├── TreeBuilder.cpp
│   ├── DirectoryTraversal.h
│   └── DirectoryTraversal.cpp
│
├── Recovery/
│   ├── FileExtractor.h
│   ├── FileExtractor.cpp
│   ├── DeletedFileScanner.h
│   ├── DeletedFileScanner.cpp
│   ├── OrphanScanner.h
│   ├── OrphanScanner.cpp
│   ├── FileCarver.h
│   ├── FileCarver.cpp
│   ├── FragmentAnalyzer.h
│   └── FragmentAnalyzer.cpp
│
├── Validation/
│   ├── FileValidator.h
│   └── FileValidator.cpp
│
├── Output/
│   ├── CSVWriter.h
│   ├── CSVWriter.cpp
│   ├── ReportWriter.h
│   └── ReportWriter.cpp
│
└── Results/
```

기존 저장소의 실제 파일 구조와 다르면 기존 구조를 우선한다.

---

# 13\. 코드 작성 규칙

1. 신규 코드 작성 전에 관련 기존 소스를 확인한다.
2. 실제 존재하는 인터페이스를 기준으로 작성한다.
3. 함수 및 변수명을 임의로 만들어 기존 모듈에 연결하지 않는다.
4. 기존 코드와 신규 코드를 명확하게 구분한다.
5. 수정할 파일의 정확한 위치를 안내한다.
6. 불필요한 전체 소스 재작성을 피한다.
7. 가능한 한 컴파일 가능한 완전한 소스를 제공한다.
8. C++17 기준으로 작성한다.
9. 기존 프로젝트의 자료형과 네이밍 규칙을 존중한다.
10. 컴파일 오류 발생 시 원인과 최소 수정 사항을 설명한다.

긴 코드는 반드시 여러 파트로 나눈다.

예시:

- Part 1: FileExtractor.h
- Part 2: FileExtractor.cpp (1/2)
- Part 3: FileExtractor.cpp (2/2)
- Part 4: 독립 테스트용 main.cpp

각 파트는 파일명과 작성 위치를 명시한다.

여러 파트를 순서대로 붙여 넣으면 완성되도록 작성한다.

코드 생략이나 '이하 동일' 표현을 사용하지 않는다.

---

# 14\. 테스트 및 검증 규칙

모듈별 독립 테스트를 실시한다.

테스트 완료 후 다음 모듈로 이동한다.

현재 단계에서는 4gb.bin의 손상 원인을 확정하거나 실제 데이터를 수정하지 않는다.

다음 사항은 금지한다.

- 비정상 파일명 임의 변경
- 시작 클러스터 임의 변경
- 파일 크기 임의 변경
- FAT1/FAT2 정상화
- 손상 FAT 체인 임의 연결
- HEX 값 임의 변경
- 원본 BIN에 쓰기 작업 수행

모든 테스트 결과는 별도 로그로 보존한다.

추출 데이터와 분석 보고서는 Results 디렉터리에 저장한다.

---

# 15\. 다음 채팅에서 바로 진행할 작업

다음 채팅에서는 아래 순서로 이어서 진행한다.

**첫 번째 작업: 기존 소스 인터페이스 확인**

확인 대상:

- Core/DiskImage.h
- Core/DiskImage.cpp
- Filesystem/BPBParser.h
- Filesystem/BPBParser.cpp
- Filesystem/FATReader.h
- Filesystem/FATReader.cpp
- Filesystem/FATChainAnalyzer.h
- Filesystem/FATChainAnalyzer.cpp
- Directory/DirectoryScanner.h
- Directory/DirectoryScanner.cpp
- Directory/TreeBuilder.h
- Directory/TreeBuilder.cpp
- Directory/LFNParser.h
- Directory/LFNParser.cpp

**두 번째 작업: Root Offset 계산 검증**

기준값:

text

```text
Partition Start: 128
Data Start Absolute Sector: 16512
Bytes Per Sector: 512
Cluster Size: 4096
Root Cluster: 2

Expected Root Offset:
8454144 bytes

Expected HEX:
0x810000
```

기존 로그의 8519680 bytes와 차이가 발생한 원인을 확인한다.

**세 번째 작업: BPBParser Primary/Backup 비교**

LBA 128과 LBA 134의 실제 BPB 필드를 비교한다.

**네 번째 작업: DirectoryTraversal 신규 구현**

기존 DirectoryScanner와 TreeBuilder 인터페이스를 활용한다.

**다섯 번째 작업: FileExtractor 신규 구현**

FAT1/FAT2 독립 분석 결과를 활용하는 읽기 전용 추출기를 구현한다.

---

# 16\. 최종 개발 목표

최종적으로 다음 기능을 갖춘 FAT32 복구 시스템을 완성한다.

- MBR 파티션 분석
- Primary/Backup VBR 비교
- BPB 검증
- FAT1/FAT2 분석
- FAT 불일치 분석
- 디렉터리 구조 복원
- SFN/LFN 이름 복원
- 정상 파일 추출
- 삭제 파일 복구
- 손상 파일 복구
- 고아 클러스터 분석
- RAW 데이터 카빙
- 단편화 파일 복구 시도
- 파일 무결성 검증
- SHA-256 생성
- CSV 생성
- 최종 복구 보고서 생성
- 복구 근거 및 신뢰도 보존

**가장 중요한 개발 원칙**

1. 확인되지 않은 구조를 가정하지 않는다.
2. 기존 분석 결과를 임의로 변경하지 않는다.
3. FAT1/FAT2 중 하나를 임의로 정상이라고 판단하지 않는다.
4. 모든 복구 결과에 분석 근거를 남긴다.
5. 원본 BIN은 항상 읽기 전용으로 유지한다.
6. 각 모듈을 독립적으로 테스트한 후 통합한다.

**다음 채팅은 기존 GitHub 소스 인터페이스 확인과 Root Offset 계산 검증부터 시작한다. FileExtractor 신규 구현은 이 검증을 마친 후 진행한다.**
