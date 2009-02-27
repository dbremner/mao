//
// Copyright 2008 Google Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef MAOCFG_H_
#define MAOCFG_H_

#include <stdio.h>
#include <list>
#include <vector>
#include <map>

#include "MaoDebug.h"
#include "MaoUnit.h"
#include "MaoPasses.h"

class BasicBlock;

typedef int BasicBlockID;

class BasicBlockEdge {
 public:
  BasicBlockEdge(BasicBlock *source, BasicBlock *dest, bool fall_through)
      : source_(source), dest_(dest), fall_through_(fall_through) { }

  bool fall_through() { return fall_through_; }
  BasicBlock *source() { return source_; }
  void set_source(BasicBlock *source) { source_ = source; }

  BasicBlock *dest() { return dest_; }
  void set_dest(BasicBlock *dest) { dest_ = dest; }

 private:
  BasicBlock *source_;
  BasicBlock *dest_;
  const bool fall_through_;
};


class BasicBlock {
 public:
  // Iterators
  //
  typedef std::vector<BasicBlockEdge *> EdgeList;
  typedef EdgeList::iterator EdgeIterator;
  typedef EdgeList::const_iterator ConstEdgeIterator;

  BasicBlock(BasicBlockID id, const char *label) : id_(id), label_(label),
                                                   first_entry_(NULL),
                                                   last_entry_(NULL) { }
  ~BasicBlock() {
    for (EdgeIterator iter = out_edges_.begin();
         iter != out_edges_.end(); ++iter) {
      delete *iter;
    }
  }

  // Getters
  //
  BasicBlockID id() const { return id_; }
  const char *label() const { return label_; }

  // Edge methods
  //
  EdgeIterator      BeginInEdges ()       { return in_edges_.begin(); }
  ConstEdgeIterator BeginInEdges () const { return in_edges_.begin(); }
  EdgeIterator      EndInEdges   ()       { return in_edges_.end(); }
  ConstEdgeIterator EndInEdges   () const { return in_edges_.end(); }
  EdgeIterator      BeginOutEdges()       { return out_edges_.begin(); }
  ConstEdgeIterator BeginOutEdges() const { return out_edges_.begin(); }
  EdgeIterator      EndOutEdges  ()       { return out_edges_.end(); }
  ConstEdgeIterator EndOutEdges  () const { return out_edges_.end(); }

  void AddInEdge(BasicBlockEdge *edge) {
    MAO_ASSERT(edge->dest() == this);
    in_edges_.push_back(edge);
  }

  void AddOutEdge(BasicBlockEdge *edge) {
    MAO_ASSERT(edge->source() == this);
    out_edges_.push_back(edge);
  }

  EdgeIterator EraseInEdge(EdgeIterator pos) {
    return in_edges_.erase(pos);
  }

  EdgeIterator EraseOutEdge(EdgeIterator pos) {
    return out_edges_.erase(pos);
  }

  // Entry methods
  SectionEntryIterator EntryBegin();
  SectionEntryIterator EntryEnd();
  void AddEntry(MaoEntry *entry);

  // Is this basic block located directly after basicblock in the section.
  bool IsArgumentAfterInSection(const BasicBlock *basicblock) const {
    // Make sure that if they are linked, both point correctly!
    MAO_ASSERT(basicblock->last_entry()->next() == NULL ||
               basicblock->last_entry()->next() != first_entry() ||
               first_entry()->prev() == basicblock->last_entry());
    return (basicblock->last_entry()->next() != NULL &&
            basicblock->last_entry()->next() == first_entry());
  }

  // Is this basic block located directly after basicblock in the section.
  bool IsArgumentBeforeInSection(const BasicBlock *basicblock) const {
    // Make sure that if they are linked, both point correctly!
    MAO_ASSERT(basicblock->first_entry()->prev() == NULL ||
               basicblock->first_entry()->prev() != last_entry() ||
               last_entry()->next() == basicblock->first_entry());
    return (basicblock->first_entry()->prev() != NULL &&
            basicblock->first_entry()->prev() == last_entry());
  }

  MaoEntry *first_entry() const { return first_entry_;}
  MaoEntry *last_entry() const { return last_entry_;}
  void set_first_entry(MaoEntry *entry)  { first_entry_ = entry;}
  void set_last_entry(MaoEntry *entry)  { last_entry_ = entry;}

 private:
  const BasicBlockID id_;
  const char *label_;

  EdgeList in_edges_;
  EdgeList out_edges_;

  // Pointers to the first and last entry of the basic block.
  MaoEntry *first_entry_;
  MaoEntry *last_entry_;
};


class CFG {
 public:
  typedef std::vector<BasicBlock *> BBVector;
  typedef std::map<const char *, BasicBlock *, ltstr> LabelToBBMap;

  explicit CFG(MaoUnit *mao_unit) : mao_unit_(mao_unit) { }
  ~CFG() {
    for (BBVector::iterator iter = basic_blocks_.begin();
         iter != basic_blocks_.end(); ++iter) {
      delete *iter;
    }
  }

  int  GetNumOfNodes() const { return basic_blocks_.size(); }
  void DumpVCG(const char *fname) const;
  void Print() const { Print(stdout); }
  void Print(FILE *out) const;

  // Construction methods
  void AddBasicBlock(BasicBlock *bb) { basic_blocks_.push_back(bb); }
  void MapBasicBlock(BasicBlock *bb) {
    MAO_ASSERT(basic_block_map_.insert(std::make_pair(bb->label(), bb)).second);
  }

  // Getter methods
  BasicBlock *GetBasicBlock(BasicBlockID id) { return basic_blocks_[id]; }
  BasicBlock *FindBasicBlock(const char *label) {
    LabelToBBMap::iterator bb = basic_block_map_.find(label);
    if (bb == basic_block_map_.end())
      return NULL;
    return bb->second;
  }
  BBVector::iterator Begin() { return basic_blocks_.begin(); }
  BBVector::const_iterator Begin() const { return basic_blocks_.begin(); }

  BBVector::iterator End() { return basic_blocks_.end(); }
  BBVector::const_iterator End() const { return basic_blocks_.end(); }

 private:
  MaoUnit  *mao_unit_;
  LabelToBBMap basic_block_map_;
  BBVector basic_blocks_;
};

class CFGBuilder : public MaoPass {
 public:
  CFGBuilder(MaoUnit *mao_unit, MaoOptions *mao_options,
             Function *function, CFG *CFG);
  void Build();

 private:
  BasicBlock *CreateBasicBlock(const char *label) {
    BasicBlock *bb = new BasicBlock(next_id_, label);
    CFG_->AddBasicBlock(bb);
    next_id_++;
    return bb;
  }

  static bool BelongsInBasicBlock(const MaoEntry *entry) {
    switch (entry->Type()) {
      case MaoEntry::INSTRUCTION: return true;
      case MaoEntry::LABEL: return true;
      case MaoEntry::DIRECTIVE: return false;
      case MaoEntry::DEBUG: return false;
      case MaoEntry::UNDEFINED:
      default:
        MAO_ASSERT(false);
        return false;
    }
  }

  bool EndsBasicBlock(const MaoEntry *entry) {
    bool has_fall_through = entry->HasFallThrough();
    bool is_control_transfer = entry->IsControlTransfer();
    bool is_call = entry->IsCall();

    // TODO(nvachhar): Parameterize this to decide whether calls end BBs
    return (is_control_transfer && !is_call) || !has_fall_through;
  }

  static BasicBlockEdge *Link(BasicBlock *source, BasicBlock *dest,
                              bool fallthrough) {
    BasicBlockEdge *edge = new BasicBlockEdge(source, dest, fallthrough);
    source->AddOutEdge(edge);
    dest->AddInEdge(edge);
    return edge;
  }

  BasicBlock *BreakUpBBAtLabel(BasicBlock *bb, LabelEntry *label);

  template <class OutputIterator>
  void GetTargets(MaoEntry *entry, OutputIterator iter) const {
    // TODO(nvachhar): Generalize this to handle indirect jumps
    if (entry->Type() == MaoEntry::INSTRUCTION) {
      InstructionEntry *insn_entry = static_cast<InstructionEntry *>(entry);
      if (!insn_entry->IsCall() && !insn_entry->IsReturn())
        *iter++ = insn_entry->GetTarget();
    }
  }

  MaoUnit  *mao_unit_;
  Function *function_;
  CFG      *CFG_;
  BasicBlockID  next_id_;
  CFG::LabelToBBMap label_to_bb_map_;
  bool      split_basic_blocks_;
};

// External entry point
void CreateCFG(MaoUnit *mao_unit, Function *function, CFG *cfg);

#endif  // MAOCFG_H_