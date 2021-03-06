//===--- MigrationState.cpp -----------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/Basic/SourceManager.h"
#include "swift/Migrator/MigrationState.h"
#include "llvm/Support/Path.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using llvm::StringRef;

using namespace swift;
using namespace swift::migrator;

#pragma mark - MigrationState

std::string MigrationState::getInputText() const {
  switch (Kind) {
    case MigrationKind::CompilerFixits:
      return cast<FixitMigrationState>(this)->getInputText();
  }
}

std::string MigrationState::getOutputText() const {
  switch (Kind) {
    case MigrationKind::CompilerFixits:
      return cast<FixitMigrationState>(this)->getOutputText();
  }
}

bool MigrationState::print(size_t StateNumber, StringRef OutDir) const {
  switch (Kind) {
    case MigrationKind::CompilerFixits:
      return cast<FixitMigrationState>(this)->print(StateNumber, OutDir);
  }
}

#pragma mark - FixitMigrationState

std::string FixitMigrationState::getInputText() const {
  return SrcMgr.getEntireTextForBuffer(InputBufferID);
}

std::string FixitMigrationState::getOutputText() const {
  return SrcMgr.getEntireTextForBuffer(OutputBufferID);
}

static bool quickDumpText(StringRef OutFilename, StringRef Text) {
  std::error_code Error;
  llvm::raw_fd_ostream FileOS(OutFilename,
                              Error, llvm::sys::fs::F_Text);
  if (FileOS.has_error()) {
    return true;
  }

  FileOS << Text;

  FileOS.flush();

  return FileOS.has_error();
}

bool FixitMigrationState::print(size_t StateNumber, StringRef OutDir) const {
  auto Failed = false;

  SmallString<256> InputFileStatePath;
  llvm::sys::path::append(InputFileStatePath, OutDir);

  {
    SmallString<256> InputStateFilenameScratch;
    llvm::raw_svector_ostream InputStateFilenameOS(InputStateFilenameScratch);
    InputStateFilenameOS << StateNumber << '-' << "FixitMigrationState";
    InputStateFilenameOS << '-' << "Input";
    llvm::sys::path::append(InputFileStatePath, InputStateFilenameOS.str());
    llvm::sys::path::replace_extension(InputFileStatePath, ".swift");
  }

  Failed |= quickDumpText(InputFileStatePath, getInputText());

  SmallString<256> OutputFileStatePath;
  llvm::sys::path::append(OutputFileStatePath, OutDir);

  {
    SmallString<256> OutputStateFilenameScratch;
    llvm::raw_svector_ostream OutputStateFilenameOS(OutputStateFilenameScratch);
    OutputStateFilenameOS << StateNumber << '-' << "FixitMigrationState";
    OutputStateFilenameOS << '-' << "Output";
    llvm::sys::path::append(OutputFileStatePath, OutputStateFilenameOS.str());
    llvm::sys::path::replace_extension(OutputFileStatePath, ".swift");
  }

  Failed |= quickDumpText(OutputFileStatePath, getOutputText());

  SmallString<256> ReplacementsStatePath;
  llvm::sys::path::append(ReplacementsStatePath, OutDir);

  {
    SmallString<256> ReplacementsStateFilenameScratch;
    llvm::raw_svector_ostream
    ReplacementsStateFilenameOS(ReplacementsStateFilenameScratch);

    ReplacementsStateFilenameOS << StateNumber << '-' << "FixitMigrationState";
    ReplacementsStateFilenameOS << '-' << "Replacements";
    llvm::sys::path::append(ReplacementsStatePath,
                            ReplacementsStateFilenameOS.str());
    llvm::sys::path::replace_extension(ReplacementsStatePath, ".remap");
  }

  Failed |= Replacement::emitRemap(ReplacementsStatePath, Replacements);

  return Failed;
}
