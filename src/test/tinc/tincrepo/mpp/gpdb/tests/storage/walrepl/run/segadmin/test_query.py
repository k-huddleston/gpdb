"""
Copyright (C) 2004-2015 Pivotal Software, Inc. All rights reserved.

This program and the accompanying materials are made available under
the terms of the under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

from tinctest import logger
from mpp.lib.PSQL import PSQL
from mpp.models import SQLTestCase

class Segadmin(SQLTestCase):
    """
    @gucs gp_create_table_random_default_distribution=off
    """

    sql_dir = 'sql'
    ans_dir = 'ans'
    out_dir = 'output'
