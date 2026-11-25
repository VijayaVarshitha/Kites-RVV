#include "assembler/parser.h"
#include "processor/registers.h"

#include <string>

namespace Kites
{
namespace
{
bool isLineEnd(const Token &token, unsigned int line)
{
    return token.type == TokenType::EOF_ || token.line_number != line;
}

unsigned int log2PowerOfTwo(unsigned int value)
{
    unsigned int result = 0;
    while (value > 1)
    {
        value >>= 1U;
        ++result;
    }
    return result;
}
} // namespace

bool Parser::parse_O_VPR_C_VPR_C_VPR()
{
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::VEC_REGISTER ||
        peekToken(4).line_number != line || peekToken(4).type != TokenType::COMMA ||
        peekToken(5).line_number != line || peekToken(5).type != TokenType::VEC_REGISTER ||
        !isLineEnd(peekToken(6), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value); // vd
    if (currentToken().value == "vmacc.vv" || currentToken().value == "vfmacc.vv")
    {
        // vmacc.vv vd, vs1, vs2  (RVV operand order for multiply-add)
        block.setRs1(peekToken(3).value);
        block.setRs2(peekToken(5).value);
    }
    else
    {
        block.setRs2(peekToken(3).value); // vs2 (second assembly operand)
        block.setRs1(peekToken(5).value); // vs1 (third assembly operand)
    }

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_VPR_C_GPR()
{
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::VEC_REGISTER ||
        peekToken(4).line_number != line || peekToken(4).type != TokenType::COMMA ||
        peekToken(5).line_number != line || peekToken(5).type != TokenType::GP_REGISTER ||
        !isLineEnd(peekToken(6), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value); // vd
    block.setRs2(peekToken(3).value); // vs2
    block.setRs1(reg_alias_to_name.at(peekToken(5).value)); // x register

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_GPR_C_VPR()
{
    // vmacc.vx vd, rs1, vs2
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::GP_REGISTER ||
        peekToken(4).line_number != line || peekToken(4).type != TokenType::COMMA ||
        peekToken(5).line_number != line || peekToken(5).type != TokenType::VEC_REGISTER ||
        !isLineEnd(peekToken(6), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value);                        // vd
    block.setRs1(reg_alias_to_name.at(peekToken(3).value)); // x register
    block.setRs2(peekToken(5).value);                       // vs2

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_GPR()
{
    // vmv.v.x vd, rs1  (vs2 field is v0)
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::GP_REGISTER ||
        !isLineEnd(peekToken(4), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value);                        // vd
    block.setRs1(reg_alias_to_name.at(peekToken(3).value)); // x register
    block.setRs2("v0");

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_VPR()
{
    // vmv.v.v vd, vs1  (vs2 field is v0)
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::VEC_REGISTER ||
        !isLineEnd(peekToken(4), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value);  // vd
    block.setRs1(peekToken(3).value); // vs1
    block.setRs2("v0");

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_I()
{
    // vmv.v.i vd, simm5  (simm5 in -16..15, stored in the rs1 field; vs2 field is v0)
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::NUM ||
        !isLineEnd(peekToken(4), line))
        return false;

    long long imm = 0;
    try
    {
        imm = std::stoll(peekToken(3).value, nullptr, 0);
    }
    catch (...)
    {
        return false;
    }
    if (imm < -16 || imm > 15)
    {
        recordError(ParseError(line, "Immediate value out of range (vmv.v.i takes -16..15)"));
        skipCurrentLine();
        return true;
    }

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value); // vd
    block.setImm(std::to_string(imm));
    block.setRs1("x0"); // unused: the immediate is encoded in its place
    block.setRs2("v0");

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_VPR_C_FPR()
{
    // vfadd.vf / vfsub.vf / vfmul.vf  vd, vs2, fs1
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::VEC_REGISTER ||
        peekToken(4).line_number != line || peekToken(4).type != TokenType::COMMA ||
        peekToken(5).line_number != line || peekToken(5).type != TokenType::FP_REGISTER ||
        !isLineEnd(peekToken(6), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value);                        // vd
    block.setRs2(peekToken(3).value);                       // vs2
    block.setRs1(reg_alias_to_name.at(peekToken(5).value)); // f register

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_FPR_C_VPR()
{
    // vfmacc.vf vd, fs1, vs2
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::FP_REGISTER ||
        peekToken(4).line_number != line || peekToken(4).type != TokenType::COMMA ||
        peekToken(5).line_number != line || peekToken(5).type != TokenType::VEC_REGISTER ||
        !isLineEnd(peekToken(6), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value);                        // vd
    block.setRs1(reg_alias_to_name.at(peekToken(3).value)); // f register
    block.setRs2(peekToken(5).value);                       // vs2

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_FPR()
{
    // vfmv.v.f vd, fs1  (vs2 field is v0)
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::FP_REGISTER ||
        !isLineEnd(peekToken(4), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value);                        // vd
    block.setRs1(reg_alias_to_name.at(peekToken(3).value)); // f register
    block.setRs2("v0");

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_GPR_C_GPR_C_VTYPE()
{
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::GP_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::GP_REGISTER ||
        peekToken(4).line_number != line || peekToken(4).type != TokenType::COMMA ||
        peekToken(5).line_number != line || peekToken(5).type != TokenType::VTYPE ||
        peekToken(5).value.size() < 2 || peekToken(5).value[0] != 'e' ||
        peekToken(6).line_number != line || peekToken(6).type != TokenType::COMMA ||
        peekToken(7).line_number != line || peekToken(7).type != TokenType::VTYPE ||
        peekToken(7).value.size() < 2 || peekToken(7).value[0] != 'm')
        return false;

    unsigned int sew = 0;
    if (peekToken(5).value == "e8")
        sew = 8;
    else if (peekToken(5).value == "e16")
        sew = 16;
    else if (peekToken(5).value == "e32")
        sew = 32;
    else if (peekToken(5).value == "e64")
        sew = 64;
    else
        return false;

    unsigned int lmul = 0;
    if (peekToken(7).value == "m1")
        lmul = 1;
    else if (peekToken(7).value == "m2")
        lmul = 2;
    else if (peekToken(7).value == "m4")
        lmul = 4;
    else if (peekToken(7).value == "m8")
        lmul = 8;
    else
        return false;

    // Policy operands are optional in this simulator. Defaults are tu,mu.
    bool tailAgnostic = false;
    bool maskAgnostic = false;
    if (peekToken(8).line_number == line && peekToken(8).type == TokenType::COMMA)
    {
        if (peekToken(9).line_number != line || peekToken(9).type != TokenType::VTYPE ||
            (peekToken(9).value != "ta" && peekToken(9).value != "tu") ||
            peekToken(10).line_number != line || peekToken(10).type != TokenType::COMMA ||
            peekToken(11).line_number != line || peekToken(11).type != TokenType::VTYPE ||
            (peekToken(11).value != "ma" && peekToken(11).value != "mu") ||
            !isLineEnd(peekToken(12), line))
            return false;
        tailAgnostic = peekToken(9).value == "ta";
        maskAgnostic = peekToken(11).value == "ma";
    }
    else if (!isLineEnd(peekToken(8), line))
    {
        return false;
    }

    const unsigned int vsew = log2PowerOfTwo(sew / 8U);
    const unsigned int vlmul = log2PowerOfTwo(lmul);
    uint32_t vtype = vlmul | (vsew << 3U);
    if (tailAgnostic)
        vtype |= (1U << 6U);
    if (maskAgnostic)
        vtype |= (1U << 7U);

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(reg_alias_to_name.at(peekToken(1).value));
    block.setRs1(reg_alias_to_name.at(peekToken(3).value));
    block.setImm(std::to_string(vtype));

    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}

bool Parser::parse_O_VPR_C_LP_GPR_RP()
{
    const unsigned int line = currentToken().line_number;
    if (peekToken(1).line_number != line || peekToken(1).type != TokenType::VEC_REGISTER ||
        peekToken(2).line_number != line || peekToken(2).type != TokenType::COMMA ||
        peekToken(3).line_number != line || peekToken(3).type != TokenType::LPAREN ||
        peekToken(4).line_number != line || peekToken(4).type != TokenType::GP_REGISTER ||
        peekToken(5).line_number != line || peekToken(5).type != TokenType::RPAREN ||
        !isLineEnd(peekToken(6), line))
        return false;

    ICUnit block;
    block.setOpcode(currentToken().value);
    block.setLineNumber(line);
    block.setInstructionIndex(instruction_index_);
    block.setRd(peekToken(1).value); // vector source for stores, destination for loads
    block.setRs1(reg_alias_to_name.at(peekToken(4).value));
    skipCurrentLine();
    intermediate_code_.emplace_back(block, true);
    instruction_number_line_number_mapping_[instruction_index_] = line;
    ++instruction_index_;
    return true;
}
} // namespace Kites
