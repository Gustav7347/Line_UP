# Line_UP

Visual Studio와 OpenGL을 사용하는 컴퓨터 그래픽스 프로젝트입니다.

## 브랜치

| 브랜치 | 용도 |
| --- | --- |
| `master` | 배포 및 안정 버전 (기본 브랜치) |
| `develop` | 개발 내용 통합 |
| `feature/a` | 기능 A 작업 |
| `feature/b` | 기능 B 작업 |

기능 개발 후 `feature/*` → `develop` 순서로 Pull Request를 만들고, 배포 준비가 끝나면 `develop` → `master`로 병합합니다.

## 시작하기

```bash
git clone https://github.com/Gustav7347/Line_UP.git
cd Line_UP
git switch develop
```

기능 A 또는 B를 작업할 때는 각각 `git switch feature/a`, `git switch feature/b`를 사용합니다.

## Visual Studio / OpenGL 파일 관리

- 솔루션(`.sln` 또는 `.slnx`), 프로젝트(`.vcxproj`, `.vcxproj.filters`), 소스, 셰이더, 텍스처, 모델은 커밋합니다.
- Wavefront `.obj` 모델은 컴파일 중간 파일과 구분할 수 있도록 `assets/` 또는 `models/` 폴더에 둡니다.
- `.vs`, `Debug`, `Release`, `x64`, `x86` 등의 빌드 폴더와 개인별 `.user` 설정은 제외합니다.
- 외부 라이브러리를 저장소에 포함할 경우 `third_party/` 같은 별도 폴더를 사용합니다. `.lib`, `.dll`, 헤더 파일은 확장자만으로 제외하지 않습니다.
- 라이브러리 경로는 `$(SolutionDir)` 또는 `$(ProjectDir)` 기준으로 설정해 개인 PC의 절대 경로가 남지 않도록 합니다.
- `.gitattributes`는 텍스트 줄바꿈과 바이너리 파일을 구분하고, `.editorconfig`는 인코딩과 들여쓰기를 맞춥니다.

현재는 저장소 초기 설정만 포함합니다. 프로젝트 파일과 OpenGL 의존성 및 빌드 방법은 실제 구현을 추가할 때 함께 기록합니다.
