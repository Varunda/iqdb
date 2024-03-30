#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <iqdb/haar_signature.h>
#include <iqdb/haar.h>
#include <iqdb/imgdb.h>
#include <iqdb/resizer.h>

namespace iqdb {

HaarSignature::HaarSignature(lumin_t avglf_, signature_t sig_) {
  std::copy(avglf_, avglf_+ 3, avglf);
  std::copy(&sig_[0][0], &sig_[0][0] + 3*40, &sig[0][0]);

  std::sort(&sig[0][0], &sig[0][NUM_COEFS]);
  std::sort(&sig[1][0], &sig[1][NUM_COEFS]);
  std::sort(&sig[2][0], &sig[2][NUM_COEFS]);
}

HaarSignature HaarSignature::from_hash(const std::string hash) {
    if (hash.size() != 5 + 2*sizeof(HaarSignature)) {
        throw param_error("Invalid hash (hash=" + hash + ")");
    }

    HaarSignature haar;
    const char* p = hash.c_str() + 5; // skip "iqdb_" prefix

    for (double& avglf : haar.avglf) {
        sscanf(p, "%16lx", reinterpret_cast<uint64_t*>(&avglf));
        p += 2 * sizeof(uint64_t);
    }

    for (int c = 0; c < 3; c++) {
        for (int16_t& coef : haar.sig[c]) {
            sscanf(p, "%4hx", reinterpret_cast<uint16_t*>(&coef));
            p += 2 * sizeof(int16_t);
        }
    }

    return haar;
}

HaarSignature HaarSignature::from_file_content(const std::string blob) {
  HaarSignature signature;
  std::vector<unsigned char> rchan(NUM_PIXELS * NUM_PIXELS);
  std::vector<unsigned char> gchan(NUM_PIXELS * NUM_PIXELS);
  std::vector<unsigned char> bchan(NUM_PIXELS * NUM_PIXELS);

  auto image = resize_image_data((const unsigned char *)blob.data(), blob.size(), NUM_PIXELS, NUM_PIXELS);

  for (int y = 0; y < NUM_PIXELS; y++) {
    for (int x = 0; x < NUM_PIXELS; x++) {
      // https://libgd.github.io/manuals/2.3.1/files/gd-c.html#gdImageGetPixel
      // https://libgd.github.io/manuals/2.3.1/files/gd-h.html#gdTrueColorGetRed
      int pixel = gdImageGetPixel(image.get(), x, y);
      rchan[x + y * NUM_PIXELS] = static_cast<unsigned char>(gdTrueColorGetRed(pixel));
      gchan[x + y * NUM_PIXELS] = static_cast<unsigned char>(gdTrueColorGetGreen(pixel));
      bchan[x + y * NUM_PIXELS] = static_cast<unsigned char>(gdTrueColorGetBlue(pixel));
    }
  }

  std::vector<Unit> cdata1(NUM_PIXELS * NUM_PIXELS);
  std::vector<Unit> cdata2(NUM_PIXELS * NUM_PIXELS);
  std::vector<Unit> cdata3(NUM_PIXELS * NUM_PIXELS);
  transformChar(rchan.data(), gchan.data(), bchan.data(), cdata1.data(), cdata2.data(), cdata3.data());
  calcHaar(cdata1.data(), cdata2.data(), cdata3.data(), signature.sig[0], signature.sig[1], signature.sig[2], signature.avglf);

  std::sort(&signature.sig[0][0], &signature.sig[0][NUM_COEFS]);
  std::sort(&signature.sig[1][0], &signature.sig[1][NUM_COEFS]);
  std::sort(&signature.sig[2][0], &signature.sig[2][NUM_COEFS]);

  return signature;
}

std::string HaarSignature::to_string() const {
  std::string str = "iqdb_";
  str.reserve(5 + sizeof(HaarSignature)*2);

  str += fmt::format("{:016x}", reinterpret_cast<const uint64_t&>(avglf[0]));
  str += fmt::format("{:016x}", reinterpret_cast<const uint64_t&>(avglf[1]));
  str += fmt::format("{:016x}", reinterpret_cast<const uint64_t&>(avglf[2]));

  for (size_t c = 0; c < 3; c++) {
    for (size_t i = 0; i < NUM_COEFS; i++) {
      str += fmt::format("{:04x}", reinterpret_cast<const uint16_t&>(sig[c][i]));
    }
  }

  return str;
}

std::string HaarSignature::to_json() const {
  return nlohmann::json({
    { "avglf", avglf },
    { "sig", sig },
  }).dump();
}

bool HaarSignature::is_grayscale() const noexcept {
  return std::abs(avglf[1]) + std::abs(avglf[2]) < 6.0 / 1000;
}

int HaarSignature::num_colors() const noexcept {
  return is_grayscale() ? 1 : 3;
}

}
