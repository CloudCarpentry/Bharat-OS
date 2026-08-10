import unittest
import os
import sys
import tempfile
import subprocess
from pathlib import Path

# Setup paths
REPO_ROOT = Path(__file__).resolve().parent.parent.parent.parent.parent
sys.path.insert(0, str(REPO_ROOT))

from tools.bidl.parser import parse_bidl, BidlParseError, SkipDialectError
from tools.bidl.bidlc import gen_types, gen_dispatch

class TestBidlTooling(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.out_dir = self.temp_dir.name

    def tearDown(self):
        self.temp_dir.cleanup()

    def write_bidl(self, content):
        path = os.path.join(self.out_dir, "test.bidl")
        with open(path, "w") as f:
            f.write(content)
        return path

    def test_valid_simple_service(self):
        content = """
        service test.service = 1 {
            rpc Ping(PingReq) -> PingResp;
        }
        message PingReq { u32 id; }
        message PingResp { u32 status; }
        """
        path = self.write_bidl(content)
        service = parse_bidl(path)
        self.assertEqual(service["name"], "test.service")
        self.assertEqual(service["id"], 1)
        self.assertEqual(len(service["rpcs"]), 1)
        self.assertEqual(service["rpcs"][0]["name"], "Ping")
        self.assertIn("PingReq", service["messages"])

    def test_implicit_service_id(self):
        content = """
        service test.service {
            rpc Ping(PingReq) -> PingResp;
        }
        """
        path = self.write_bidl(content)
        service = parse_bidl(path)
        self.assertEqual(service["name"], "test.service")
        self.assertEqual(service["id"], 0)

    def test_local_cap_descriptor_shadowing(self):
        # Local cap_descriptor should resolve to struct cap_descriptor, not wire type
        content = """
        service cap.service = 1 {
            rpc Test(TestReq) -> TestResp;
        }
        struct cap_descriptor {
            u64 val;
        }
        struct TestReq {
            cap_descriptor desc;
        }
        """
        path = self.write_bidl(content)
        service = parse_bidl(path)

        gen_types(service, self.out_dir)
        gen_dispatch(service, self.out_dir)

        types_path = os.path.join(self.out_dir, "cap_service_types.h")
        with open(types_path) as f:
            c = f.read()
            # Since cap_descriptor is locally defined, we should not include wire_types.h
            self.assertNotIn("bharat_cap_wire_t", c)
            self.assertIn("struct cap_descriptor desc;", c)

    def test_missing_brace_fails(self):
        content = """
        service test = 1 {
            rpc Ping(PingReq) -> PingResp
        """
        path = self.write_bidl(content)
        with self.assertRaises(BidlParseError):
            parse_bidl(path)

    def test_duplicate_rpc_fails(self):
        content = """
        service test = 1 {
            rpc Ping(PingReq) -> PingResp;
            rpc Ping(PingReq2) -> PingResp2;
        }
        """
        path = self.write_bidl(content)
        with self.assertRaises(BidlParseError):
            parse_bidl(path)

    def test_unknown_type_codegen_fails(self):
        content = """
        service test = 1 {
        }
        struct Msg {
            unknown_type foo;
        }
        """
        path = self.write_bidl(content)
        service = parse_bidl(path)
        with self.assertRaises(Exception):
            gen_types(service, self.out_dir)

    def test_skip_dialect(self):
        content = """
        namespace test.dialect;
        interface MyInterface {
        }
        """
        path = self.write_bidl(content)
        with self.assertRaises(SkipDialectError):
            parse_bidl(path)

    def test_generated_source_compiles_standalone_and_coexists(self):
        # We'll generate two services and compile them together
        content1 = """
        service service1 = 1 {
            rpc Call1(Req1) -> Resp1;
        }
        struct Req1 { u32 v; }
        struct Resp1 { u32 v; }
        """
        path1 = self.write_bidl(content1)
        srv1 = parse_bidl(path1)
        gen_types(srv1, self.out_dir)
        gen_dispatch(srv1, self.out_dir)

        content2 = """
        service service2 = 2 {
            rpc Call2(Req2) -> Resp2;
        }
        struct Req2 { u64 v; }
        struct Resp2 { u64 v; }
        """
        path2 = self.write_bidl(content2)
        srv2 = parse_bidl(path2)
        gen_types(srv2, self.out_dir)
        gen_dispatch(srv2, self.out_dir)

        # Write a dummy main to link them
        main_c = os.path.join(self.out_dir, "main.c")
        with open(main_c, "w") as f:
            f.write("#include <stdint.h>\n")
            f.write("extern int service1_dispatch(uint16_t op);\n")
            f.write("extern int service2_dispatch(uint16_t op);\n")
            f.write("int main() {\n")
            f.write("    service1_dispatch(1);\n")
            f.write("    service2_dispatch(1);\n")
            f.write("    return 0;\n")
            f.write("}\n")

        # Compile! We just need standard CC.
        subprocess.run(["gcc", "-I", self.out_dir,
                        os.path.join(self.out_dir, "service1_dispatch.c"),
                        os.path.join(self.out_dir, "service2_dispatch.c"),
                        main_c, "-o", os.path.join(self.out_dir, "test_bin")],
                       check=True)

    def test_deterministic_generation(self):
        content = """
        service det = 1 {
            rpc C(Req) -> Resp;
            rpc A(Req) -> Resp;
            rpc B(Req) -> Resp;
        }
        struct B { u32 x; }
        struct A { u32 y; }
        struct C { u32 z; }
        """
        path = self.write_bidl(content)
        service = parse_bidl(path)

        gen_types(service, self.out_dir)
        gen_dispatch(service, self.out_dir)

        with open(os.path.join(self.out_dir, "det_types.h")) as f:
            out1 = f.read()

        gen_types(service, self.out_dir)
        with open(os.path.join(self.out_dir, "det_types.h")) as f:
            out2 = f.read()

        self.assertEqual(out1, out2)

    def test_unknown_opcode_returns_failure(self):
        content = """
        service test = 1 {
            rpc Op(Req) -> Resp;
        }
        struct Req { u32 v; }
        struct Resp { u32 v; }
        """
        path = self.write_bidl(content)
        srv = parse_bidl(path)
        gen_types(srv, self.out_dir)
        gen_dispatch(srv, self.out_dir)

        # Test compile a small snippet verifying BH_BIDL_DISPATCH_UNKNOWN_OPCODE
        main_c = os.path.join(self.out_dir, "main.c")
        with open(main_c, "w") as f:
            f.write('#include "test_types.h"\n')
            f.write("extern int test_dispatch(uint16_t opcode);\n")
            f.write("int main() {\n")
            f.write("    if (test_dispatch(999) != BH_BIDL_DISPATCH_UNKNOWN_OPCODE) return 1;\n")
            f.write("    return 0;\n")
            f.write("}\n")

        subprocess.run(["gcc", "-I", self.out_dir,
                        os.path.join(self.out_dir, "test_dispatch.c"),
                        main_c, "-o", os.path.join(self.out_dir, "test_bin2")],
                       check=True)
        subprocess.run([os.path.join(self.out_dir, "test_bin2")], check=True)

if __name__ == '__main__':
    unittest.main()
