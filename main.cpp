#include <cmath>
#include <cstddef>
#include <expected>
#include <print>
#include <ranges>
#include <string_view>

std::expected<size_t, std::string> is_valid_str(std::string_view fp_str) {
    size_t strlen = fp_str.length();

    if (strlen != 32 && strlen != 64)
        return std::unexpected("Invalid string length.");

    for (auto c : fp_str)
        if (c != '0' && c != '1')
            return std::unexpected("Malformed binary string.");

    return fp_str.length();
}

std::expected<void, std::string> showcase_conversion(std::string_view fp_str) {
    enum class Float754 {
        singleP,
        doubleP,
    };

    Float754 flt_type{};

    if (auto len = is_valid_str(fp_str); len) {
        flt_type = (len == 32) ? Float754::singleP : Float754::doubleP;
    } else {
        return std::unexpected(len.error());
    }

    int exp_first_idx{1};
    int exp_last_idx{};

    int mantissa_first_idx{};
    int mantissa_last_idx{};

    int exp_sub_magnitude{};
    int exp_upper_bound{};

    if (flt_type == Float754::singleP) {
        exp_last_idx       = 9;
        mantissa_first_idx = 9;
        mantissa_last_idx  = 32;

        exp_sub_magnitude = 127;
        exp_upper_bound   = 255;
    } else {
        exp_last_idx       = 12;
        mantissa_first_idx = 12;
        mantissa_last_idx  = 64;

        exp_sub_magnitude = 1023;
        exp_upper_bound   = 2047;
    }

    const char sign_bit{fp_str[0]};
    std::string_view exponent_str{fp_str.begin() + exp_first_idx, fp_str.begin() + exp_last_idx};
    std::string_view mantissa_str{fp_str.begin() + mantissa_first_idx, fp_str.begin() + mantissa_last_idx};

    std::println("Sign-Bit: {}", sign_bit);
    std::println("Exponent: {}", exponent_str);
    std::println("Mantissa: {}\n", mantissa_str);

    // 1. Resolve mantissa
    // AKA the significand. This basically tells you the 'actual number' so to speak
    double mantissa{0};
    int power{2}; // start at 2 because the leading `1.` is implicitly added later on; this grants us an additional bit
                  // to work with ;; Caveat: subnormal numbers (exponent is 0).

    for (auto c : mantissa_str) {
        if (c == '1') {
            std::println("Adding to mantissa: 1 / {} = {}", power, 1.0 / power);
            mantissa += (1.0 / power);
        }

        power *= 2;
    }

    // CHECK: Is this a standards violation? Fuck if I know.

    printf("Mantissa: %.100g\n\n", mantissa);

    // 2. Determine exponent
    // This denotes *where* the binary point would be placed relative to the start of the digit
    int exponent{0};
    power = 0;

    for (auto c : exponent_str | std::views::reverse) {
        if (c == '1') {
            std::println("Adding to exponent: 2^{} = {}", power, (1 << power));
            exponent += (1 << power);
        }

        power++;
    }

    std::println("\nExponent: {}", exponent);

    // 3. Cater to edge cases
    bool is_subnormal{false};

    if (exponent == 0) {
        if (mantissa == 0) {
            std::println("\nDecimal Value: {}0", sign_bit == '1' ? '-' : '+');
            return {};
        } else {
            is_subnormal = true;
            std::println("\nSubnormal number detected.");
            std::println("Subracting {} from exponent instead of {}.\n", exp_sub_magnitude - 1, exp_sub_magnitude);
        }
    } else if (exponent == exp_upper_bound) {

        if (mantissa == 0) {
            std::println("\nDecimal Value: {}inf", sign_bit == '1' ? '-' : '+');
        } else {
            std::println("\nNaN detected.");
        }

        return {};
    }

    // 4. Bring it all together
    exponent -= exp_sub_magnitude;

    if (is_subnormal) [[unlikely]] { // Probably?
        exponent += 1;
    } else {
        mantissa += 1;
        std::println("Mantissa after adding implicit 1: {}", mantissa);
    }

    std::println("Exponent after 'normalizing': {}", exponent);

    if (sign_bit == '1')
        mantissa *= -1;

    printf("\nDecimal value: %.100g * 2^%d = %.100g\n", mantissa, exponent, mantissa * powf(2, exponent));

    return {};
}

int main(int argc, char *argv[]) {
#ifdef DEBUG
    std::string_view num{"00000100010000000000000000000000"};
#else
    if (argc != 2) {
        std::println(stderr, "Usage: {} <binarystring>", argv[0]);
        return 4;
    }

    std::string_view num{argv[1]};
#endif

    auto ret = showcase_conversion(num);

    if (!ret) {
        std::println(stderr, "{}", ret.error());
        return 4;
    }

    return 0;
}
