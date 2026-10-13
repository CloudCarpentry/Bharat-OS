import pytest
from pathlib import Path
import tempfile
from tools.docs.check_front_matter import validate_front_matter

def create_temp_md(content):
    f = tempfile.NamedTemporaryFile("w+", suffix=".md", delete=False)
    f.write(content)
    f.close()
    return Path(f.name)

def test_valid_front_matter():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags:
  - doc
  - test
see_also:
  - other_doc.md
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert violations == []

def test_missing_key():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags:
  - doc
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert "Missing required metadata key: 'see_also'" in violations

def test_tags_not_list():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags: doc
see_also: []
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert any("Metadata key 'tags' must be a list" in v for v in violations)

def test_tags_empty_list():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags: []
see_also: []
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert any("Metadata key 'tags' cannot be an empty list" in v for v in violations)

def test_tags_invalid_elements():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags:
  - doc
  - 123
  - ""
see_also: []
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert any("Metadata key 'tags' elements must be strings" in v for v in violations)
    assert any("Metadata key 'tags' elements cannot be blank strings" in v for v in violations)

def test_see_also_empty_list():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags:
  - doc
see_also: []
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert violations == []

def test_see_also_invalid_elements():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags:
  - doc
see_also:
  - ""
  - 123
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert any("Metadata key 'see_also' elements cannot be blank strings" in v for v in violations)
    assert any("Metadata key 'see_also' elements must be strings" in v for v in violations)

def test_last_updated_invalid_format():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: Jan 1, 2026
tags:
  - doc
see_also: []
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert any("Metadata key 'last_updated' must be in YYYY-MM-DD format" in v for v in violations)

def test_last_updated_string_format():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: "2026-08-12"
tags:
  - doc
see_also: []
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert violations == []

def test_malformed_yaml():
    content = """---
title: Valid Doc
status: Proposed
owner: Doc Team
last_updated: 2026-08-12
tags: [unclosed list
see_also: []
---
Body content
"""
    path = create_temp_md(content)
    violations = validate_front_matter(path)
    path.unlink()
    assert any("Failed to parse YAML front matter" in v for v in violations)
