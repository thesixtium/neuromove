include_guard(GLOBAL)

include(FetchContent)
set(TINY_BCI_GIT_TOKEN "" CACHE STRING "GitHub PAT for cloning private TinyBCI repo")

if (TINY_BCI_GIT_TOKEN)
  set(TINY_BCI_REPO_URL "https://${TINY_BCI_GIT_TOKEN}@github.com/BCI-Games/TinyBCI.git")
else()
  set(TINY_BCI_REPO_URL "https://github.com/BCI-Games/TinyBCI.git")
endif()

FetchContent_Declare(
  tiny_bci
  GIT_REPOSITORY ${TINY_BCI_REPO_URL}
  GIT_TAG 1590909aaba5bdf6d082996b6a4ba9559a55afcc
)
FetchContent_MakeAvailable(tiny_bci)