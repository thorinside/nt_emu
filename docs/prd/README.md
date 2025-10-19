# PRD Epic Documentation

This directory contains individual epic documentation files following the standard epic template format.

## Epic Files

### Epic 1: Finish DistingNT Feature Emulation
- **File:** `epic-1.md`
- **Status:** Draft
- **Priority:** Critical (Blocker for Epic 2)
- **Effort:** 3-4 story points
- **Duration:** 1 week
- **Stories:** 2 (1.1, 1.2)

### Epic 2: Multiplatform VCV Plugin Release
- **File:** `epic-2.md`
- **Status:** Draft
- **Priority:** Critical (Enables ecosystem)
- **Effort:** 20-28 story points
- **Duration:** 3 weeks
- **Stories:** 8 (2.1-2.8)


## Epic Template Structure

Each epic file follows this structure:

```markdown
# Epic N: Title

**Epic ID:** EPIC-NNN-SHORT-NAME
**Status:** Draft | Planned | In Progress | Complete
**Priority:** Critical | High | Medium | Low
**Effort:** X-Y story points
**Duration:** N weeks

## Epic Goal
[Clear objective and scope]

## Epic Value Proposition
**For Users:** [User value]
**For Developers:** [Developer value]

## Success Criteria
- [ ] Measurable outcomes

## Stories in Epic N
### Story N.M: Title
**As a** [role],
**I want** [capability],
**So that** [benefit].
**Story Points:** X-Y hours

## Story Dependencies
[Dependency diagram]

## Timeline
[With/without parallelization]

## Risk Assessment
[Risk table]

## Related Documentation
[Links to other docs]
```

## Usage with Orchestra

Orchestra automatically discovers epic information from this directory:

```bash
# View all epics
orchestra report

# Filter to specific epic
orchestra report --epic 1

# Run stories in an epic
orchestra run 1.1
```

## Usage with BMAD Workflows

These epic files are used by BMAD workflows for context and planning:

- **PRD Workflow:** References epics during requirements planning
- **Story Creation:** Pulls epic context when generating stories
- **Solution Architecture:** Uses epic structure for component mapping

## Related Documentation

- **Main PRD:** `/docs/prd.md` - High-level product requirements
- **Full Epic Breakdown:** `/docs/epics.md` - Complete epic details in single file
- **Stories:** `/docs/stories/` - Individual story files
- **Solution Architecture:** `/docs/solution-architecture.md` - Technical design

---

**Last Updated:** 2025-10-19
**Total Epics:** 2
**Total Stories:** 10
