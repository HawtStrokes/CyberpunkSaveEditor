#pragma once
#include <Windows.h>
#include <inttypes.h>

#include <tools/rtti_dumper_dll/mem.hpp>

namespace dumper {

inline PIMAGE_NT_HEADERS image_nt_header(uintptr_t module_address)
{
  const auto pDosHdr = (PIMAGE_DOS_HEADER)module_address;
  if (pDosHdr->e_magic == IMAGE_DOS_SIGNATURE && pDosHdr->e_lfanew >= sizeof(PIMAGE_NT_HEADERS))
  {
    const auto pNtHdr = (PIMAGE_NT_HEADERS)((LPCSTR)module_address + pDosHdr->e_lfanew);
    if (pNtHdr->Signature == IMAGE_NT_SIGNATURE)
      return pNtHdr;
  }
  return nullptr;
}

inline address_range get_code_section_range(uintptr_t image_base)
{
  PIMAGE_NT_HEADERS pNtHdr = image_nt_header(image_base);
  if (!pNtHdr)
  {
    SPDLOG_ERROR("error getting pNtHdr");
    return {};
  }

  const PIMAGE_SECTION_HEADER pSectionHdr = IMAGE_FIRST_SECTION(pNtHdr);
  for (SSIZE_T i = 0; i < pNtHdr->FileHeader.NumberOfSections; ++i)
  {
    const IMAGE_SECTION_HEADER& SectionHdr = pSectionHdr[i];
    SPDLOG_INFO("section {}", (LPCSTR)SectionHdr.Name);
    if (0 == _strcmpi(".text", (LPCSTR)SectionHdr.Name))
    {
      uintptr_t start = (uintptr_t)((LPCSTR)image_base + SectionHdr.VirtualAddress);
      return { start, start + SectionHdr.Misc.VirtualSize };
    }
  }

  return {};
}

inline std::vector<uintptr_t> find_pattern_in_game_text(std::wstring masked_pattern)
{
  static address_range range = []()
  {
    uintptr_t image_base = (uintptr_t)GetModuleHandle(0);
    address_range ret = get_code_section_range(image_base);

    SPDLOG_INFO("game text range: {:016X}-{:016X}", ret.beg, ret.end);

    if (!ret)
    {
      SPDLOG_ERROR("couldn't find game text section");
    }

    return ret;

  }();

  if (!range)
  {
    return {};
  }

  std::vector<uintptr_t> ret = find_pattern_in(range, masked_pattern);
  return ret;
}

template <size_t N>
inline std::vector<uintptr_t> find_pattern_in_game_text(const wchar_t(&masked_pattern)[N])
{
  return find_pattern_in_game_text(std::wstring(masked_pattern, N-1));
}

} // namespace dumper

