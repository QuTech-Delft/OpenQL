import typing
import CQASMParser
from multiprocessing import Pool
from threading import Lock

def longestCommonPrefix(s1, s2):
    for i in range(min(len(s1), len(s2))):
        if s1[i] != s2[i]:
            return i
    return min(len(s1), len(s2))

class SuffixTree:
    def __init__(self):
        self.instructions = []
        self.position = -1
        self.children = []
    
    def __repr__(self):
        return "{ " + repr(self.instructions) + ("(" + str(self.position) + ")" if self.position != -1 else "") + " - " + ", ".join(map(repr, self.children)) + "}"

    def add(self, instr: list[CQASMParser.Instruction], position: int):
        for c in self.children:
            if c is not None:
                cp = longestCommonPrefix(instr, c.instructions)
                if cp == len(c.instructions):
                    c.add(instr[len(c.instructions):], position)
                    return
                
                if cp > 0:
                    newChild1 = SuffixTree()
                    newChild1.instructions = c.instructions[cp:]
                    newChild1.children = c.children
                    newChild1.position = c.position

                    newChild2 = SuffixTree()
                    newChild2.instructions = instr[cp:]
                    newChild2.position = position
                    
                    c.instructions = c.instructions[:cp]
                    c.position = -1
                    c.children = [newChild1, newChild2]
                    return


        
        newChild1 = SuffixTree()
        newChild1.instructions = instr
        newChild1.position = position

        if (self.children == []):
            self.children = [newChild1, None]
        else:
            self.children += [newChild1]

    def deepestInternalNode(self) -> (list[CQASMParser.Instruction], int):
        if self.children == []:
            return ([], 0)
        
        subresults = map(lambda c: c.deepestInternalNode(), filter(lambda c: c is not None, self.children))

        m = max(subresults, key = lambda x: len(x[0]))

        if m[0] == []:
            return (self.instructions, len(self.children))
        
        return (self.instructions + m[0], m[1])

            




# naive construction of suffix tree
def longestRepeatingSubcircuit(c: list[CQASMParser.Instruction]):
    t = SuffixTree()
    for i in range(len(c)):
        start = len(c) - i - 1
        t.add(instr=c[start:], position=start)

    return t.deepestInternalNode()

files = [
    "/shares/bulk/plehenaff/medina/ex-1_166.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=19990_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/mlp4_245.qasm",
    "/shares/bulk/plehenaff/medina/0410184_169.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_500CYC_QSE_7.qasm",
    "/shares/bulk/plehenaff/medina/cycle10_2_110.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=2989_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=39997_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=992_2qbf=081_1.qasm",
    "/shares/bulk/plehenaff/medina/15_enc.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_800CYC_QSE_4.qasm",
    "/shares/bulk/plehenaff/medina/dc1_220.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=29989_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=57_2qbf=011_1.qasm",
    "/shares/bulk/plehenaff/medina/q=9_s=19991_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_100CYC_QSE_1.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_900CYC_QSE_3.qasm",
    "/shares/bulk/plehenaff/medina/dc2_222.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=39989_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=5997_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/q=9_s=2991_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_10CYC_TFL_1.qasm",
    "/shares/bulk/plehenaff/medina/3_17_13.qasm",
    "/shares/bulk/plehenaff/medina/decod24-v1_41.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=49989_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=59997_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/q=9_s=51_2qbf=012_1.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_15CYC_TFL_2.qasm",
    "/shares/bulk/plehenaff/medina/4_49_16.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=49_2qbf=061_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=97_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=9_s=51_2qbf=059_1.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_20CYC_TFL_3.qasm",
    "/shares/bulk/plehenaff/medina/4gt10-v1_81.qasm",
    "/shares/bulk/plehenaff/medina/deutsch_n2.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=89_2qbf=022_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=997_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=9_s=91_2qbf=088_1.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_35CYC_TFL_6.qasm",
    "/shares/bulk/plehenaff/medina/4gt11_82.qasm",
    "/shares/bulk/plehenaff/medina/dist_223.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=989_2qbf=081_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=9997_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=9_s=991_2qbf=091_1.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_40CYC_TFL_7.qasm",
    "/shares/bulk/plehenaff/medina/4gt12-v0_87.qasm",
    "/shares/bulk/plehenaff/medina/dnn_n16.qasm",
    "/shares/bulk/plehenaff/medina/q=12_s=19988_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=19996_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/qaoa_n16.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_500CYC_QSE_3.qasm",
    "/shares/bulk/plehenaff/medina/4gt13_92.qasm",
    "/shares/bulk/plehenaff/medina/dnn_n2.qasm",
    "/shares/bulk/plehenaff/medina/q=12_s=29988_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=19996_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/qaoa_n6.qasm",
    "/shares/bulk/plehenaff/medina/16QBT_700CYC_QSE_5.qasm",
    "/shares/bulk/plehenaff/medina/4gt4-v0_72.qasm",
    "/shares/bulk/plehenaff/medina/dnn_n8.qasm",
    "/shares/bulk/plehenaff/medina/q=12_s=49988_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=2996_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/qec_en_n5.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_100CYC_QSE_8.qasm",
    "/shares/bulk/plehenaff/medina/4gt5_75.qasm",
    "/shares/bulk/plehenaff/medina/error_correctiond3_n5.qasm",
    "/shares/bulk/plehenaff/medina/q=12_s=59988_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=29996_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/qec_sm_n5.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_400CYC_QSE_8.qasm",
    "/shares/bulk/plehenaff/medina/4mod5-bdd_287.qasm",
    "/shares/bulk/plehenaff/medina/ex-1_166.qasm",
    "/shares/bulk/plehenaff/medina/q=12_s=9988_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=39996_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/qft_n15.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_1D2_0.qasm",
    "/shares/bulk/plehenaff/medina/4mod7-v0_94.qasm",
    "/shares/bulk/plehenaff/medina/ex3_229.qasm",
    "/shares/bulk/plehenaff/medina/q=13_s=19987_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=49996_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/qft_n20.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_1D2_5.qasm",
    "/shares/bulk/plehenaff/medina/53QBT_700CYC_QSE_3.qasm",
    "/shares/bulk/plehenaff/medina/f2_232.qasm",
    "/shares/bulk/plehenaff/medina/q=13_s=29987_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=49996_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/quantum_volume_n5.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_2D2_0.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_10CYC_QSE_9.qasm",
    "/shares/bulk/plehenaff/medina/fredkin_n3.qasm",
    "/shares/bulk/plehenaff/medina/q=13_s=49987_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=56_2qbf=032_1.qasm",
    "/shares/bulk/plehenaff/medina/radd_250.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_2D2_5.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_15CYC_QSE_8.qasm",
    "/shares/bulk/plehenaff/medina/graycode6_47.qasm",
    "/shares/bulk/plehenaff/medina/q=13_s=59987_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=96_2qbf=052_1.qasm",
    "/shares/bulk/plehenaff/medina/rd32_270.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_2D2_9.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_200CYC_QSE_7.qasm",
    "/shares/bulk/plehenaff/medina/grover_n2.qasm",
    "/shares/bulk/plehenaff/medina/q=13_s=9987_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=996_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/rd53_311.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_3D2_0.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_25CYC_QSE_6.qasm",
    "/shares/bulk/plehenaff/medina/grover_n3.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=19986_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/q=4_s=9996_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/rd73_140.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_3D2_5.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_35CYC_QSE_4.qasm",
    "/shares/bulk/plehenaff/medina/ham7_104.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=29986_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=19995_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/rd73_252.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_3D2_9.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_40CYC_QSE_3.qasm",
    "/shares/bulk/plehenaff/medina/hwb7_59.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=39986_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=19995_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/rd84_142.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_4D2_0.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_500CYC_QSE_0.qasm",
    "/shares/bulk/plehenaff/medina/inc_237.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=49986_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=2995_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/root_255.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_5D2_1.qasm",
    "/shares/bulk/plehenaff/medina/54QBT_800CYC_QSE_7.qasm",
    "/shares/bulk/plehenaff/medina/ising_model_13.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=5986_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=39995_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/sao2_257.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_6D2_2.qasm",
    "/shares/bulk/plehenaff/medina/9symml_195.qasm",
    "/shares/bulk/plehenaff/medina/iswap_n2.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=5986_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=49995_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/sat_n11.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_7D2_3.qasm",
    "/shares/bulk/plehenaff/medina/C17_204.qasm",
    "/shares/bulk/plehenaff/medina/life_238.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=59986_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=55_2qbf=087_1.qasm",
    "/shares/bulk/plehenaff/medina/seca_n11.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_0D1_8D2_4.qasm",
    "/shares/bulk/plehenaff/medina/adder_n10.qasm",
    "/shares/bulk/plehenaff/medina/linearsolver_n3.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=986_2qbf=051_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=95_2qbf=095_1.qasm",
    "/shares/bulk/plehenaff/medina/sf_274.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_1D1_1D2_5.qasm",
    "/shares/bulk/plehenaff/medina/adder_n4.qasm",
    "/shares/bulk/plehenaff/medina/lpn_n5.qasm",
    "/shares/bulk/plehenaff/medina/q=14_s=9986_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/q=5_s=9995_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/shor_15.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_1D1_2D2_6.qasm",
    "/shares/bulk/plehenaff/medina/adr4_197.qasm",
    "/shares/bulk/plehenaff/medina/majority_239.qasm",
    "/shares/bulk/plehenaff/medina/q=15_s=19985_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=6_s=19994_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/shor_35.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_1D1_3D2_7.qasm",
    "/shares/bulk/plehenaff/medina/aj-e11_165.qasm",
    "/shares/bulk/plehenaff/medina/max46_240.qasm",
    "/shares/bulk/plehenaff/medina/q=15_s=29985_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=6_s=2994_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/simon_n6.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_1D1_4D2_8.qasm",
    "/shares/bulk/plehenaff/medina/alu-bdd_288.qasm",
    "/shares/bulk/plehenaff/medina/miller_11.qasm",
    "/shares/bulk/plehenaff/medina/q=15_s=49985_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=6_s=29994_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/sqn_258.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_1D1_5D2_9.qasm",
    "/shares/bulk/plehenaff/medina/alu-v0_26.qasm",
    "/shares/bulk/plehenaff/medina/mini-alu_167.qasm",
    "/shares/bulk/plehenaff/medina/q=15_s=59985_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=6_s=49994_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/sqrt8_260.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_1D1_6D2_0.qasm",
    "/shares/bulk/plehenaff/medina/alu-v1_28.qasm",
    "/shares/bulk/plehenaff/medina/misex1_241.qasm",
    "/shares/bulk/plehenaff/medina/q=15_s=985_2qbf=051_1.qasm",
    "/shares/bulk/plehenaff/medina/q=6_s=54_2qbf=022_1.qasm",
    "/shares/bulk/plehenaff/medina/squar5_261.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_1D1_7D2_1.qasm",
    "/shares/bulk/plehenaff/medina/alu-v2_30.qasm",
    "/shares/bulk/plehenaff/medina/mlp4_245.qasm",
    "/shares/bulk/plehenaff/medina/q=15_s=9985_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/q=6_s=94_2qbf=053_1.qasm",
    "/shares/bulk/plehenaff/medina/square_root_7.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_2D1_2D2_3.qasm",
    "/shares/bulk/plehenaff/medina/alu-v2_31.qasm",
    "/shares/bulk/plehenaff/medina/mod10_171.qasm",
    "/shares/bulk/plehenaff/medina/q=16_s=19984_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/q=6_s=9994_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/sym9_148.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_2D1_3D2_4.qasm",
    "/shares/bulk/plehenaff/medina/alu-v3_34.qasm",
    "/shares/bulk/plehenaff/medina/mod5adder_127.qasm",
    "/shares/bulk/plehenaff/medina/q=16_s=29984_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=19993_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/sys6-v0_111.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_2D1_4D2_5.qasm",
    "/shares/bulk/plehenaff/medina/alu-v4_36.qasm",
    "/shares/bulk/plehenaff/medina/mod5d2_64.qasm",
    "/shares/bulk/plehenaff/medina/q=16_s=49984_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=2993_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/teleportation_n3.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_2D1_5D2_6.qasm",
    "/shares/bulk/plehenaff/medina/basis_change_n3.qasm",
    "/shares/bulk/plehenaff/medina/multipler_n15.qasm",
    "/shares/bulk/plehenaff/medina/q=16_s=59984_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=2993_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/toffoli_n3.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_2D1_6D2_7.qasm",
    "/shares/bulk/plehenaff/medina/basis_trotter_n4.qasm",
    "/shares/bulk/plehenaff/medina/multiply_n13.qasm",
    "/shares/bulk/plehenaff/medina/q=16_s=984_2qbf=051_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=29993_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/urf5_280.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_3D1_1D2_8.qasm",
    "/shares/bulk/plehenaff/medina/bell_n4.qasm",
    "/shares/bulk/plehenaff/medina/one-two-three-v1_99.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=19983_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=39993_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/variational_n4.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_3D1_2D2_9.qasm",
    "/shares/bulk/plehenaff/medina/bigadder_n18.qasm",
    "/shares/bulk/plehenaff/medina/plus63mod4096_163.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=2983_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=53_2qbf=034_1.qasm",
    "/shares/bulk/plehenaff/medina/vbeAdder_1b.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_3D1_3D2_0.qasm",
    "/shares/bulk/plehenaff/medina/bv_n14.qasm",
    "/shares/bulk/plehenaff/medina/pm1_249.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=29983_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=59993_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/vbeAdder_5b.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_3D1_4D2_1.qasm",
    "/shares/bulk/plehenaff/medina/bv_n19.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=19990_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=43_2qbf=028_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=93_2qbf=054_1.qasm",
    "/shares/bulk/plehenaff/medina/vqe_uccsd_n4.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_3D1_5D2_2.qasm",
    "/shares/bulk/plehenaff/medina/cat_state_n4.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=19990_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=49983_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=7_s=993_2qbf=081_1.qasm",
    "/shares/bulk/plehenaff/medina/vqe_uccsd_n6.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_4D1_1D2_3.qasm",
    "/shares/bulk/plehenaff/medina/cm82a_208.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=2990_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=5983_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=19992_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/vqe_uccsd_n8.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_4D1_2D2_4.qasm",
    "/shares/bulk/plehenaff/medina/cnt3-5_179.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=29990_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=59983_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=2992_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/wim_266.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_4D1_3D2_5.qasm",
    "/shares/bulk/plehenaff/medina/co14_215.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=39990_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=983_2qbf=031_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=2992_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/wstate_n3.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_4D1_4D2_6.qasm",
    "/shares/bulk/plehenaff/medina/con1_216.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=49990_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/q=17_s=9983_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=29992_2qbf=05_1.qasm",
    "/shares/bulk/plehenaff/medina/xor5_254.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_5D1_1D2_7.qasm",
    "/shares/bulk/plehenaff/medina/cuccaroAdder_10b.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=50_2qbf=096_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=19997_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=39992_2qbf=08_1.qasm",
    "/shares/bulk/plehenaff/medina/z4_268.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_5D1_2D2_8.qasm",
    "/shares/bulk/plehenaff/medina/cuccaroAdder_1b.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=90_2qbf=011_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=2997_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=49992_2qbf=09_1.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_5D1_3D2_9.qasm",
    "/shares/bulk/plehenaff/medina/cuccaroMultiplier_10b.qasm",
    "/shares/bulk/plehenaff/medina/q=10_s=990_2qbf=091_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=2997_2qbf=02_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=52_2qbf=104_1.qasm",
    "/shares/bulk/plehenaff/medina/20QBT_45CYC_6D1_2D2_1.qasm",
    "/shares/bulk/plehenaff/medina/cuccaroMultiplier_1b.qasm",
    "/shares/bulk/plehenaff/medina/q=11_s=19989_2qbf=01_1.qasm",
    "/shares/bulk/plehenaff/medina/q=3_s=29997_2qbf=03_1.qasm",
    "/shares/bulk/plehenaff/medina/q=8_s=92_2qbf=011_1.qasm",
]

# ast = CQASMParser.parseCQASMFile("/shares/bulk/plehenaff/medina/q=10_s=19990_2qbf=02_1.qasm")


lock = Lock()

def writeToFile(s: str):
    lock.acquire()
    with open("result.txt", "a") as writer:
        writer.write(s)
    lock.release()

def processFile(tupleArg):
    fileIndex, fileName = tupleArg
    toWrite=""
    try:
        ast = CQASMParser.parseCQASMFile(fileName)
    except Exception as e:
        print(f"File {fileName} gave error: {e}")
        return

    toWrite+=f"File: {fileName} ({len(ast.subcircuits)} subcircuit(s))\n\n"

    for index, subcircuit in enumerate(ast.subcircuits):
        print(f"Processing file n. {fileIndex+1} / {len(files)}, subcircuit {index+1} / {len(ast.subcircuits)}...")
        res = longestRepeatingSubcircuit(subcircuit.instructions)
        if res[0] != []:
            toWrite+=f"\tLongest repeating gates in subcircuit {index} contains {len(res[0])} gates for a total of {len(subcircuit.instructions)} gates in the subcircuit and occurs {res[1]} times:\n"
            toWrite+='\n'.join(map(str, res[0]))
        else:
            toWrite+=f"\Subcircuit {index} does not contain any repeated substring of gates of length >= 2"

        toWrite+='\n'
    toWrite+="\n\n"
    writeToFile(toWrite)

# processFile((0, files[0]))
with Pool(8) as p:
    p.map(processFile, enumerate(files))
